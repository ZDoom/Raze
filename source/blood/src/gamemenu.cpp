//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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
#include "build.h"
#include "compat.h"
#include "mouse.h"
#include "common_game.h"
#include "blood.h"
#include "config.h"
#include "gamemenu.h"
#include "globals.h"
#include "inifile.h"
#include "levels.h"
#include "menu.h"
#include "qav.h"
#include "resource.h"
#include "view.h"

CMenuTextMgr gMenuTextMgr;
CGameMenuMgr gGameMenuMgr;

extern CGameMenuItemPicCycle itemSorryPicCycle;
extern CGameMenuItemQAV itemBloodQAV;

CMenuTextMgr::CMenuTextMgr()
{
    at0 = -1;
}

static char buffer[21][45];

void CMenuTextMgr::DrawText(const char *pString, int nFont, int x, int y, int nShade, int nPalette, bool shadow )
{
    viewDrawText(nFont, pString, x, y, nShade, nPalette, 0, shadow);
}

void CMenuTextMgr::GetFontInfo(int nFont, const char *pString, int *pXSize, int *pYSize)
{
    if (nFont < 0 || nFont >= 5)
        return;
    viewGetFontInfo(nFont, pString, pXSize, pYSize);
}

bool CGameMenuMgr::m_bInitialized = false;
bool CGameMenuMgr::m_bActive = false;
bool CGameMenuMgr::m_bFirstPush = true;

CGameMenuMgr::CGameMenuMgr()
{
    dassert(!m_bInitialized);
    m_bInitialized = true;
    Clear();
}

CGameMenuMgr::~CGameMenuMgr()
{
    m_bInitialized = false;
    Clear();
}

void CGameMenuMgr::InitializeMenu(void)
{
    if (pActiveMenu)
    {
        CGameMenuEvent event;
        event.at0 = kMenuEventInit;
        event.at2 = 0;
        pActiveMenu->Event(event);
    }
}

void CGameMenuMgr::DeInitializeMenu(void)
{
    if (pActiveMenu)
    {
        CGameMenuEvent event;
        event.at0 = kMenuEventDeInit;
        event.at2 = 0;
        pActiveMenu->Event(event);
    }
}

bool CGameMenuMgr::Push(CGameMenu *pMenu, int nItem)
{
    if (nMenuPointer == 0)
    {
        mouseReadAbs(&m_prevmousepos, &g_mouseAbs);
        m_mouselastactivity = -M_MOUSETIMEOUT;
        m_mousewake_watchpoint = 0;
        mouseLockToWindow(0);
        if (m_bFirstPush)
            m_bFirstPush = false;
        else
            mouseMoveToCenter();
    }
    dassert(pMenu != NULL);
    if (nMenuPointer == 8)
        return false;
    pActiveMenu = pMenuStack[nMenuPointer] = pMenu;
    nMenuPointer++;
    if (nItem >= 0)
        pMenu->SetFocusItem(nItem);
    m_bActive = true;
    gInputMode = INPUT_MODE_1;
    InitializeMenu();
    m_menuchange_watchpoint = 1;
    m_mousecaught = 1;
    return true;
}

void CGameMenuMgr::Pop(void)
{
    if (nMenuPointer > 0)
    {
        DeInitializeMenu();
        nMenuPointer--;
        if (nMenuPointer == 0)
            Deactivate();
        else
            pActiveMenu = pMenuStack[nMenuPointer-1];

        m_menuchange_watchpoint = 1;
    }
    m_mousecaught = 1;
}

void CGameMenuMgr::PostPop(void)
{
    m_postPop = true;
}

void CGameMenuMgr::Draw(void)
{
    if (pActiveMenu)
    {
        pActiveMenu->Draw();
        viewUpdatePages();
    }

    if (m_postPop)
    {
        Pop();
        m_postPop = false;
    }

    int32_t mousestatus = mouseReadAbs(&m_mousepos, &g_mouseAbs);
    if (mousestatus && g_mouseClickState == MOUSE_PRESSED)
        m_mousedownpos = m_mousepos;

    int16_t mousetile = 1043; // red arrow
    if (tilesiz[mousetile].x > 0 && mousestatus)
    {
        if (!MOUSEACTIVECONDITION)
            m_mousewake_watchpoint = 1;

        if (MOUSEACTIVECONDITIONAL(mouseAdvanceClickState()) || m_mousepos.x != m_prevmousepos.x || m_mousepos.y != m_prevmousepos.y)
        {
            m_prevmousepos = m_mousepos;
            m_mouselastactivity = totalclock;
        }
        else
            m_mousewake_watchpoint = 0;

        m_mousecaught = 0;
    }
    else
    {
        m_mouselastactivity = -M_MOUSETIMEOUT;

        m_mousewake_watchpoint = 0;
    }

    // Display the mouse cursor, except on touch devices.
    if (MOUSEACTIVECONDITION && !m_bFirstPush)
    {
        vec2_t cursorpos = { m_mousepos.x + (7 << 16), m_mousepos.y + (6 << 16) };

        if ((unsigned) mousetile < MAXTILES)
        {
            int32_t scale = 65536;
            int16_t rotate = 768;
            uint32_t stat = 2|4|8;
            int8_t alpha = MOUSEALPHA; //CURSORALPHA;
            rotatesprite_fs_alpha(cursorpos.x, cursorpos.y, scale, rotate, mousetile, 0, 0, stat, alpha);
        }
    }
    else
        g_mouseClickState = MOUSE_IDLE;
}

void CGameMenuMgr::Clear(void)
{
    pActiveMenu = NULL;
    memset(pMenuStack, 0, sizeof(pMenuStack));
    nMenuPointer = 0;
    m_postPop = false;
}

void CGameMenuMgr::Process(void)
{
    if (!pActiveMenu)
        return;

    if (m_menuchange_watchpoint > 0)
        m_menuchange_watchpoint++;

    CGameMenuEvent event;
    event.at0 = 0;
    event.at2 = 0;
    char key;
    if (!pActiveMenu->MouseEvent(event) && (key = keyGetScan()) != 0 )
    {
        keyFlushScans();
        keyFlushChars();
        event.at2 = key;
        switch (key)
        {
        case sc_Escape:
            event.at0 = kMenuEventEscape;
            break;
        case sc_Tab:
            if (keystatus[sc_LeftShift] || keystatus[sc_RightShift])
                event.at0 = kMenuEventUp;
            else
                event.at0 = kMenuEventDown;
            break;
        case sc_UpArrow:
        case sc_kpad_8:
            event.at0 = kMenuEventUp;
            gGameMenuMgr.m_mouselastactivity = -M_MOUSETIMEOUT;
            break;
        case sc_DownArrow:
        case sc_kpad_2:
            event.at0 = kMenuEventDown;
            gGameMenuMgr.m_mouselastactivity = -M_MOUSETIMEOUT;
            break;
        case sc_Enter:
        case sc_kpad_Enter:
            event.at0 = kMenuEventEnter;
            break;
        case sc_Space:
            event.at0 = kMenuEventSpace;
            break;
        case sc_LeftArrow:
        case sc_kpad_4:
            event.at0 = kMenuEventLeft;
            break;
        case sc_RightArrow:
        case sc_kpad_6:
            event.at0 = kMenuEventRight;
            break;
        case sc_Delete:
        case sc_kpad_Period:
            event.at0 = kMenuEventDelete;
            break;
        case sc_BackSpace:
            event.at0 = kMenuEventBackSpace;
            break;
        default:
            event.at0 = kMenuEventKey;
            break;
        }
    }
    if (pActiveMenu->Event(event))
        Pop();

    if (m_menuchange_watchpoint >= 3)
        m_menuchange_watchpoint = 0;
}

void CGameMenuMgr::Deactivate(void)
{
    Clear();
    keyFlushScans();
    keyFlushChars();
    m_bActive = false;

    mouseLockToWindow(1);
    gInputMode = INPUT_MODE_0;
}

bool CGameMenuMgr::MouseOutsideBounds(vec2_t const * const pos, const int32_t x, const int32_t y, const int32_t width, const int32_t height)
{
    return pos->x < x || pos->x >= x + width || pos->y < y || pos->y >= y + height;
}

CGameMenu::CGameMenu()
{
    m_nItems = 0;
    m_nFocus = at8 = -1;
    atc = 0;
}

CGameMenu::CGameMenu(int unk)
{
    m_nItems = 0;
    m_nFocus = at8 = -1;
    atc = unk;
}

CGameMenu::~CGameMenu()
{
    if (!atc)
        return;
    for (int i = 0; i < m_nItems; i++)
    {
        if (pItemList[i] != &itemBloodQAV && pItemList[i] != &itemSorryPicCycle)
            delete pItemList[i];
    }
}

void CGameMenu::InitializeItems(CGameMenuEvent &event)
{
    for (int i = 0; i < m_nItems; i++)
    {
        pItemList[i]->Event(event);
    }
}

void CGameMenu::Draw(void)
{
    for (int i = 0; i < m_nItems; i++)
    {
        if (pItemList[i]->pPreDrawCallback)
            pItemList[i]->pPreDrawCallback(pItemList[i]);
        if (i == m_nFocus || (i != m_nFocus && !pItemList[i]->bNoDraw))
            pItemList[i]->Draw();
    }
}

bool CGameMenu::Event(CGameMenuEvent &event)
{
    if (m_nItems <= 0)
        return true;
    switch (event.at0)
    {
    case kMenuEventInit:
    case kMenuEventDeInit:
        if (at8 >= 0)
            m_nFocus = at8;
        InitializeItems(event);
        return false;
    }
    if (m_nFocus < 0)
        return true;
    return pItemList[m_nFocus]->Event(event);
}

void CGameMenu::Add(CGameMenuItem *pItem, bool active)
{
    dassert(pItem != NULL);
    dassert(m_nItems < kMaxGameMenuItems);
    pItemList[m_nItems] = pItem;
    pItem->pMenu = this;
    if (active)
        m_nFocus = at8 = m_nItems;
    m_nItems++;
}

void CGameMenu::SetFocusItem(int nItem)
{
    dassert(nItem >= 0 && nItem < m_nItems && nItem < kMaxGameMenuItems);
    if (CanSelectItem(nItem))
        m_nFocus = at8 = nItem;
}

void CGameMenu::SetFocusItem(CGameMenuItem *pItem)
{
    for (int i = 0; i < m_nItems; i++)
        if (pItemList[i] == pItem)
        {
            SetFocusItem(i);
            break;
        }
}

bool CGameMenu::CanSelectItem(int nItem)
{
    dassert(nItem >= 0 && nItem < m_nItems && nItem < kMaxGameMenuItems);
    return pItemList[nItem]->bCanSelect && pItemList[nItem]->bEnable;
}

void CGameMenu::FocusPrevItem(void)
{
    dassert(m_nFocus >= -1 && m_nFocus < m_nItems && m_nFocus < kMaxGameMenuItems);
    int t = m_nFocus;
    do
    {
        m_nFocus--;
        if (m_nFocus < 0)
            m_nFocus += m_nItems;
        if (CanSelectItem(m_nFocus))
            break;
    } while(t != m_nFocus);
}

void CGameMenu::FocusNextItem(void)
{
    dassert(m_nFocus >= -1 && m_nFocus < m_nItems && m_nFocus < kMaxGameMenuItems);
    int t = m_nFocus;
    do
    {
        m_nFocus++;
        if (m_nFocus >= m_nItems)
            m_nFocus = 0;
        if (CanSelectItem(m_nFocus))
            break;
    } while(t != m_nFocus);
}

bool CGameMenu::IsFocusItem(CGameMenuItem *pItem)
{
    if (m_nFocus < 0)
        return false;
    dassert(m_nFocus >= 0 && m_nFocus < m_nItems && m_nFocus < kMaxGameMenuItems);
    return pItemList[m_nFocus] == pItem;
}

bool CGameMenu::MouseEvent(CGameMenuEvent &event)
{
    if (m_nItems <= 0 || m_nFocus < 0)
        return true;
    return pItemList[m_nFocus]->MouseEvent(event);
}

CGameMenuItem::CGameMenuItem()
{
    m_pzText = NULL;
    m_nX = m_nY = m_nWidth = 0;
    bCanSelect = 1;
    bEnable = 1;
    m_nFont = -1;
    pMenu = NULL;
    bNoDraw = 0;
    pPreDrawCallback = NULL;
}

CGameMenuItem::~CGameMenuItem()
{
}

bool CGameMenuItem::Event(CGameMenuEvent &event)
{
    switch (event.at0)
    {
    case kMenuEventEscape:
        return true;
    case kMenuEventUp:
        pMenu->FocusPrevItem();
        break;
    case kMenuEventDown:
        pMenu->FocusNextItem();
        break;
    }
    return false;
}

bool CGameMenuItem::MouseEvent(CGameMenuEvent &event)
{
    event.at0 = kMenuEventNone;
    if (MOUSEINACTIVECONDITIONAL(MOUSE_GetButtons()&LEFT_MOUSE))
    {
        event.at0 = kMenuEventEnter;
        MOUSE_ClearButton(LEFT_MOUSE);
    }
    else if (MOUSE_GetButtons()&RIGHT_MOUSE)
    {
        event.at0 = kMenuEventEscape;
        MOUSE_ClearButton(RIGHT_MOUSE);
    }
#if 0
    else if (MOUSEINACTIVECONDITIONAL((MOUSE_GetButtons()&LEFT_MOUSE) && (MOUSE_GetButtons()&WHEELUP_MOUSE)))
    {
        MOUSE_ClearButton(WHEELUP_MOUSE);
        event.bAutoAim = kMenuEventScrollLeft;
    }
    else if (MOUSEINACTIVECONDITIONAL((MOUSE_GetButtons()&LEFT_MOUSE) && (MOUSE_GetButtons()&WHEELDOWN_MOUSE)))
    {
        MOUSE_ClearButton(WHEELDOWN_MOUSE);
        event.bAutoAim = kMenuEventScrollRight;
    }
#endif
    else if (MOUSE_GetButtons()&WHEELUP_MOUSE)
    {
        MOUSE_ClearButton(WHEELUP_MOUSE);
        event.at0 = kMenuEventUp;
    }
    else if (MOUSE_GetButtons()&WHEELDOWN_MOUSE)
    {
        MOUSE_ClearButton(WHEELDOWN_MOUSE);
        event.at0 = kMenuEventDown;
    }
    return event.at0 != kMenuEventNone;
}

CGameMenuItemText::CGameMenuItemText()
{
    m_pzText = 0;
    bEnable = 0;
}

CGameMenuItemText::CGameMenuItemText(const char *a1, int a2, int a3, int a4, int a5)
{
    m_nWidth = 0;
    m_pzText = a1;
    m_nFont = a2;
    m_nX = a3;
    m_nY = a4;
    at20 = a5;
    bEnable = 0;
}

void CGameMenuItemText::Draw(void)
{
    if (m_pzText)
    {
        int width;
        int x = m_nX;
        switch (at20)
        {
        case 1:
            gMenuTextMgr.GetFontInfo(m_nFont, m_pzText, &width, NULL);
            x = m_nX-width/2;
            break;
        case 2:
            gMenuTextMgr.GetFontInfo(m_nFont, m_pzText, &width, NULL);
            x = m_nX-width;
            break;
        }
        gMenuTextMgr.DrawText(m_pzText,m_nFont, x, m_nY, -128, 0, false);
    }
}

CGameMenuItemTitle::CGameMenuItemTitle()
{
    m_pzText = 0;
    bEnable = 0;
}

CGameMenuItemTitle::CGameMenuItemTitle(const char *a1, int a2, int a3, int a4, int a5)
{
    m_nWidth = 0;
    m_pzText = a1;
    m_nFont = a2;
    m_nX = a3;
    m_nY = a4;
    at20 = a5;
    bEnable = 0;
}

void CGameMenuItemTitle::Draw(void)
{
    if (m_pzText)
    {
        int height;
        gMenuTextMgr.GetFontInfo(m_nFont, NULL, NULL, &height);
        if (at20 >= 0)
            rotatesprite(320<<15, m_nY<<16, 65536, 0, at20, -128, 0, 78, 0, 0, xdim-1, ydim-1);
        viewDrawText(m_nFont, m_pzText, m_nX, m_nY-height/2, -128, 0, 1, false);
    }
}

CGameMenuItemZBool::CGameMenuItemZBool()
{
    at20 = false;
    m_pzText = 0;
    at21 = "On";
    at25 = "Off";
}

CGameMenuItemZBool::CGameMenuItemZBool(const char *a1, int a2, int a3, int a4, int a5, bool a6, void(*a7)(CGameMenuItemZBool *), const char *a8, const char *a9)
{
    m_pzText = a1;
    m_nFont = a2;
    m_nX = a3;
    m_nY = a4;
    m_nWidth = a5;
    at20 = a6;
    at29 = a7;
    if (!a8)
        at21 = "On";
    else
        at21 = a8; 
    if (!a9)
        at25 = "Off";
    else
        at25 = a9; 
}

void CGameMenuItemZBool::Draw(void)
{
    int shade = bEnable ? 32 : 48;
    int pal = bEnable ? 0 : 5;
    if (pMenu->IsFocusItem(this))
        shade = 32-(totalclock&63);
    if (m_pzText)
        gMenuTextMgr.DrawText(m_pzText, m_nFont, m_nX, m_nY, shade, pal, false);
    const char *value = at20 ? at21 : at25;
    int width, height;
    gMenuTextMgr.GetFontInfo(m_nFont, value, &width, &height);
    gMenuTextMgr.DrawText(value, m_nFont, m_nWidth-1+m_nX-width, m_nY, shade, pal, false);
    int mx = m_nX<<16;
    int my = m_nY<<16;
    int mw = m_nWidth<<16;
    int mh = height<<16;
    if (bEnable && MOUSEACTIVECONDITIONAL(!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_mousepos, mx, my, mw, mh)))
    {
        if (MOUSEWATCHPOINTCONDITIONAL(!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_prevmousepos, mx, my, mw, mh)))
        {
            pMenu->SetFocusItem(this);
        }

        if (!gGameMenuMgr.m_mousecaught && g_mouseClickState == MOUSE_RELEASED && !gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_mousedownpos, mx, my, mw, mh))
        {
            pMenu->SetFocusItem(this);

            CGameMenuEvent event = { kMenuEventEnter, 0 };

            gGameMenuMgr.m_mousecaught = 1;

            if (Event(event))
                gGameMenuMgr.PostPop();
        }
    }
}

bool CGameMenuItemZBool::Event(CGameMenuEvent &event)
{
    switch (event.at0)
    {
    case kMenuEventEnter:
    case kMenuEventSpace:
        at20 = !at20;
        if (at29)
            at29(this);
        return false;
    }
    return CGameMenuItem::Event(event);
}

CGameMenuItemChain::CGameMenuItemChain()
{
    m_pzText = NULL;
    at24 = NULL;
    at28 = -1;
    at2c = NULL;
    at30 = 0;
}

CGameMenuItemChain::CGameMenuItemChain(const char *a1, int a2, int a3, int a4, int a5, int a6, CGameMenu *a7, int a8, void(*a9)(CGameMenuItemChain *), int a10)
{
    m_pzText = a1;
    m_nFont = a2;
    m_nX = a3;
    m_nY = a4;
    m_nWidth = a5;
    at20 = a6;
    at24 = a7;
    at28 = a8;
    at2c = a9;
    at30 = a10;
}

void CGameMenuItemChain::Draw(void)
{
    if (!m_pzText) return;
    int shade = bEnable ? 32 : 48;
    int pal = bEnable ? 0 : 5;
    if (pMenu->IsFocusItem(this))
        shade = 32-(totalclock&63);
    int width, height;
    int x = m_nX;
    int y = m_nY;
    gMenuTextMgr.GetFontInfo(m_nFont, m_pzText, &width, &height);
    switch (at20)
    {
    case 1:
        x = m_nX+m_nWidth/2-width/2;
        break;
    case 2:
        x = m_nX+m_nWidth-1-width;
        break;
    case 0:
    default:
        break;
    }
    gMenuTextMgr.DrawText(m_pzText, m_nFont, x, m_nY, shade, pal, true);
    if (bEnable && MOUSEACTIVECONDITIONAL(!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_mousepos, x<<16, y<<16, width<<16, height<<16)))
    {
        if (MOUSEWATCHPOINTCONDITIONAL(!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_prevmousepos, x<<16, y<<16, width<<16, height<<16)))
        {
            pMenu->SetFocusItem(this);
        }

        if (!gGameMenuMgr.m_mousecaught && g_mouseClickState == MOUSE_RELEASED && !gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_mousedownpos, x<<16, y<<16, width<<16, height<<16))
        {
            pMenu->SetFocusItem(this);

            CGameMenuEvent event = { kMenuEventEnter, 0 };

            gGameMenuMgr.m_mousecaught = 1;

            if (Event(event))
                gGameMenuMgr.PostPop();
        }
    }
}

bool CGameMenuItemChain::Event(CGameMenuEvent &event)
{
    switch (event.at0)
    {
    case kMenuEventEnter:
        if (at2c)
            at2c(this);
        if (at24)
            gGameMenuMgr.Push(at24, at28);
        return false;
    }
    return CGameMenuItem::Event(event);
}

CGameMenuItem7EA1C::CGameMenuItem7EA1C()
{
    m_pzText = NULL;
    at24 = NULL;
    at28 = -1;
    at2c = NULL;
    at30 = 0;
    at34 = NULL;
    at38[0] = 0;
    at48[0] = 0;
}

CGameMenuItem7EA1C::CGameMenuItem7EA1C(const char *a1, int a2, int a3, int a4, int a5, const char *a6, const char *a7, int a8, int a9, void(*a10)(CGameMenuItem7EA1C *), int a11)
{
    m_pzText = a1;
    m_nFont = a2;
    m_nX = a3;
    m_nY = a4;
    m_nWidth = a5;
    at20 = a8;
    at28 = a9;
    at2c = a10;
    at30 = a11;
    strncpy(at38, a6, 15);
    strncpy(at48, a7, 15);
}

void CGameMenuItem7EA1C::Draw(void)
{
    if (!m_pzText) return;
    int shade = bEnable ? 32 : 48;
    int pal = bEnable ? 0 : 5;
    if (pMenu->IsFocusItem(this))
        shade = 32-(totalclock&63);
    int width;
    int x = m_nX;
    switch (at20)
    {
    case 1:
        gMenuTextMgr.GetFontInfo(m_nFont, m_pzText, &width, NULL);
        x = m_nX+m_nWidth/2-width/2;
        break;
    case 2:
        gMenuTextMgr.GetFontInfo(m_nFont, m_pzText, &width, NULL);
        x = m_nX+m_nWidth-1-width;
        break;
    case 0:
    default:
        break;
    }
    gMenuTextMgr.DrawText(m_pzText, m_nFont, x, m_nY, shade, pal, true);
}

void CGameMenuItem7EA1C::Setup(void)
{
    if (!at34 || !at24)
        return;
    if (!at34->SectionExists(at48))
        return;
    const char *title = at34->GetKeyString(at48, "Title", at48);
    at24->Add(new CGameMenuItemTitle(title, 1, 160, 20, 2038), false);
    at24->Add(&itemSorryPicCycle, true);
    int y = 40;
    for (int i = 0; i < 21; i++)
    {
        sprintf(buffer[i], "Line%d", i+1);
        if (!at34->KeyExists(at48, buffer[i]))
            break;
        const char *line = at34->GetKeyString(at48, buffer[i], NULL);
        if (line)
        {
            if (*line == 0)
            {
                y += 10;
                continue;
            }
            at24->Add(new CGameMenuItemText(line, 1, 160, y, 1), false);
            y += 20;
        }
    }
    at24->Add(&itemBloodQAV, false);
}

bool CGameMenuItem7EA1C::Event(CGameMenuEvent &event)
{
    switch (event.at0)
    {
    case kMenuEventEnter:
    {
        if (at2c)
            at2c(this);
        if (at24)
            delete at24;
        at24 = new CGameMenu(1);
        DICTNODE *pRes = gGuiRes.Lookup(at38, "MNU");
        if (pRes)
        {
            at34 = new IniFile(gGuiRes.Load(pRes));
            Setup();
        }
        if (at24)
            gGameMenuMgr.Push(at24, at28);
        return false;
    }
    case kMenuEventDeInit:
        if (at34)
        {
            delete at34;
            at34 = NULL;
        }
        if (at24)
        {
            delete at24;
            at24 = NULL;
        }
        return false;
    }
    return CGameMenuItem::Event(event);
}

CGameMenuItem7EE34::CGameMenuItem7EE34()
{
    m_pzText = NULL;
    at28 = NULL;
    at20 = -1;
    at2c = NULL;
}

CGameMenuItem7EE34::CGameMenuItem7EE34(const char *a1, int a2, int a3, int a4, int a5, int a6)
{
    m_pzText = NULL;
    at28 = NULL;
    at20 = -1;
    at2c = NULL;
    m_nFont = a2;
    m_nX = a3;
    m_pzText = a1;
    m_nY = a4;
    m_nWidth = a5;
    at24 = a6;
}

void CGameMenuItem7EE34::Draw(void)
{
    if (!m_pzText) return;
    int shade = bEnable ? 32 : 48;
    int pal = bEnable ? 0 : 5;
    if (pMenu->IsFocusItem(this))
        shade = 32-(totalclock&63);
    int width;
    int x = m_nX;
    switch (at24)
    {
    case 1:
        gMenuTextMgr.GetFontInfo(m_nFont, m_pzText, &width, NULL);
        x = m_nX+m_nWidth/2-width/2;
        break;
    case 2:
        gMenuTextMgr.GetFontInfo(m_nFont, m_pzText, &width, NULL);
        x = m_nX+m_nWidth-1-width;
        break;
    case 0:
    default:
        break;
    }
    gMenuTextMgr.DrawText(m_pzText, m_nFont, x, m_nY, shade, pal, true);
}

extern void SetVideoModeOld(CGameMenuItemChain *pItem);

void CGameMenuItem7EE34::Setup(void)
{
    if (!at28)
        return;
    at28->Add(new CGameMenuItemTitle("Video Mode", 1, 160, 20, 2038), false);
    if (!at2c)
    {
        at2c = new CGameMenu(1);
        at2c->Add(new CGameMenuItemTitle(" Mode Change ", 1, 160, 20, 2038), false);
        at2c->Add(&itemSorryPicCycle, true);
        CGameMenuItem *pItem1 = new CGameMenuItemText("VIDEO MODE WAS SET", 1, 160, 90, 1);
        CGameMenuItem *pItem2 = new CGameMenuItemText("NOT ALL MODES Work correctly", 1, 160, 110, 1);
        CGameMenuItem *pItem3 = new CGameMenuItemText("Press ESC to exit", 3, 160, 140, 1);
        at2c->Add(pItem1, false);
        pItem1->bEnable = 0;
        at2c->Add(pItem2, false);
        pItem2->bEnable = 0;
        at2c->Add(pItem3, true);
        pItem3->bEnable = 1;
        at2c->Add(&itemBloodQAV, false);
    }
    sprintf(buffer[0], "640 x 480 (default)");
    int y = 40;
    at28->Add(new CGameMenuItemChain(buffer[0], 3, 0, y, 320, 1, at2c, -1, SetVideoModeOld, validmodecnt), true);
    y += 20;
    for (int i = 0; i < validmodecnt && i < 20; i++)
    {
        sprintf(buffer[i+1], "%d x %d", validmode[i].xdim, validmode[i].ydim);
        at28->Add(new CGameMenuItemChain(buffer[i+1], 3, 0, y, 320, 1, at2c, -1, SetVideoModeOld, i), false);
        if (validmodecnt > 10)
            y += 7;
        else
            y += 15;
    }
    at28->Add(&itemBloodQAV, false);
}

bool CGameMenuItem7EE34::Event(CGameMenuEvent &event)
{
    switch (event.at0)
    {
    case kMenuEventEnter:
        if (at28)
            delete at28;
        at28 = new CGameMenu(1);
        Setup();
        if (at28)
            gGameMenuMgr.Push(at28, at20);
        return false;
    case kMenuEventDeInit:
        if (at28)
        {
            delete at28;
            at28 = 0;
        }
        return false;
    }
    return CGameMenuItem::Event(event);
}

CGameMenuItemChain7F2F0::CGameMenuItemChain7F2F0()
{
    at34 = -1;
}

CGameMenuItemChain7F2F0::CGameMenuItemChain7F2F0(char *a1, int a2, int a3, int a4, int a5, int a6, CGameMenu *a7, int a8, void(*a9)(CGameMenuItemChain *), int a10) :
    CGameMenuItemChain(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)
{
    at34 = a10;
}

bool CGameMenuItemChain7F2F0::Event(CGameMenuEvent &event)
{
    switch (event.at0)
    {
    case kMenuEventEnter:
        if (at34 > -1)
            gGameOptions.nEpisode = at34;
        return CGameMenuItemChain::Event(event);
    }
    return CGameMenuItem::Event(event);
}

CGameMenuItemBitmap::CGameMenuItemBitmap()
{
    m_pzText = NULL;
}

CGameMenuItemBitmap::CGameMenuItemBitmap(const char *a1, int a2, int a3, int a4, int a5)
{
    m_pzText = a1;
    m_nFont = a2;
    m_nX = a3;
    m_nY = a4;
    at20 = a5;
}

void CGameMenuItemBitmap::Draw(void)
{
    int shade = bEnable ? 32 : 48;
    int pal = bEnable ? 0 : 5;
    if (bEnable && pMenu->IsFocusItem(this))
        shade = 32-(totalclock&63);
    int x = m_nX;
    int y = m_nY;
    if (m_pzText)
    {
        int height;
        gMenuTextMgr.DrawText(m_pzText, m_nFont, x, y, shade, pal, false);
        gMenuTextMgr.GetFontInfo(m_nFont, NULL, NULL, &height);
        y += height + 2;
    }
    rotatesprite(x<<15,y<<15, 65536, 0, at20, 0, 0, 82, 0, 0, xdim-1,ydim-1);
}

bool CGameMenuItemBitmap::Event(CGameMenuEvent &event)
{
    if (bEnable && pMenu->IsFocusItem(this))
        pMenu->FocusNextItem();
    return CGameMenuItem::Event(event);
}

CGameMenuItemBitmapLS::CGameMenuItemBitmapLS()
{
    m_pzText = NULL;
}

CGameMenuItemBitmapLS::CGameMenuItemBitmapLS(const char *a1, int a2, int a3, int a4, int a5)
{
    at24 = -1;
    m_pzText = a1;
    m_nFont = a2;
    m_nX = a3;
    m_nY = a4;
    at28 = a5;
}

void CGameMenuItemBitmapLS::Draw(void)
{
    int shade = bEnable ? 32 : 48;
    int pal = bEnable ? 0 : 5;
    if (bEnable && pMenu->IsFocusItem(this))
        shade = 32-(totalclock&63);
    int x = m_nX;
    int y = m_nY;
    if (m_pzText)
    {
        int height;
        gMenuTextMgr.DrawText(m_pzText, m_nFont, x, y, shade, pal, false);
        gMenuTextMgr.GetFontInfo(m_nFont, NULL, NULL, &height);
        y += height + 2;
    }
    char stat;
    int16_t ang;
    int picnum;
    if (at24 == -1)
    {
        stat = 66;
        ang = 0;
        picnum = at28;
    }
    else
    {
        ang = 512;
        stat = 70;
        picnum = at24;
    }
    rotatesprite(200<<15,215<<15,32768, ang, picnum, 0, 0, stat, 0, 0, xdim-1, ydim-1);
}

bool CGameMenuItemBitmapLS::Event(CGameMenuEvent &event)
{
    if (bEnable && pMenu->IsFocusItem(this))
        pMenu->FocusNextItem();
    return CGameMenuItem::Event(event);
}

CGameMenuItemKeyList::CGameMenuItemKeyList()
{
    m_pzText = NULL;
    m_nFont = 3;
    m_nX = 0;
    m_nY = 0;
    nRows = 0;
    nTopDelta = 0;
    nFocus = 0;
    nGameFuncs = 0;
    bScan = false;
}

CGameMenuItemKeyList::CGameMenuItemKeyList(const char *a1, int a2, int a3, int a4, int a5, int a6, int a7, void(*a8)(CGameMenuItemKeyList *))
{
    nTopDelta = 0;
    nFocus = 0;
    bScan = false;
    m_pzText = a1;
    m_nFont = a2;
    m_nX = a3;
    m_nY = a4;
    m_nWidth = a5;
    nRows = a6;
    pCallback = a8;
    nGameFuncs = a7;
}

void CGameMenuItemKeyList::Scan(void)
{
    KB_FlushKeyboardQueue();
    KB_FlushKeyboardQueueScans();
    KB_ClearKeysDown();
    KB_LastScan = 0;
    bScan = true;
}

extern uint8_t KeyboardKeys[NUMGAMEFUNCTIONS][2];
void CGameMenuItemKeyList::Draw(void)
{
    char buffer[40], buffer2[40];
    int width, height;
    int shade;
    gMenuTextMgr.GetFontInfo(m_nFont, NULL, NULL, &height);
    int y = m_nY;
    int k = nFocus - nTopDelta;
    int nNewFocus = nFocus;
    bool bClick = false;
    for (int i = 0; i < nRows; i++, y += height, k++)
    {
        char key1, key2;
        key1 = KeyboardKeys[k][0];
        key2 = KeyboardKeys[k][1];
        const char *sKey1 = key1 == sc_Tilde ? "Tilde" : KB_ScanCodeToString(key1);
        const char *sKey2 = key2 == sc_Tilde ? "Tilde" : KB_ScanCodeToString(key2);
        sprintf(buffer, "%s", CONFIG_FunctionNumToName(k));
        if (key2 == 0 || key2 == 0xff)
        {
            if (key1 == 0 || key1 == 0xff)
                sprintf(buffer2, "????");
            else
                sprintf(buffer2, "%s", sKey1);
        }
        else
            sprintf(buffer2, "%s or %s", sKey1, sKey2);
        
        if (k == nFocus)
        {
            shade = 32;
            if (pMenu->IsFocusItem(this))
                shade = 32-(totalclock&63);
            viewDrawText(3, buffer, m_nX, y, shade, 0, 0, false);
            const char *sVal;
            if (bScan && (gGameClock & 32))
                sVal = "____";
            else
                sVal = buffer2;
            gMenuTextMgr.GetFontInfo(m_nFont, sVal, &width, 0);
            viewDrawText(m_nFont, sVal, m_nX+m_nWidth-1-width, y, shade, 0, 0, false);
        }
        else
        {
            viewDrawText(3, buffer, m_nX, y, 24, 0, 0, false);
            gMenuTextMgr.GetFontInfo(m_nFont, buffer2, &width, 0);
            viewDrawText(m_nFont, buffer2, m_nX+m_nWidth-1-width, y, 24, 0, 0, false);
        }
        int mx = m_nX<<16;
        int my = y<<16;
        int mw = m_nWidth<<16;
        int mh = height<<16;
        if (bEnable && MOUSEACTIVECONDITIONAL(!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_mousepos, mx, my, mw, mh)))
        {
            if (MOUSEWATCHPOINTCONDITIONAL(!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_prevmousepos, mx, my, mw, mh)))
            {
                nNewFocus = k;
            }

            if (!gGameMenuMgr.m_mousecaught && g_mouseClickState == MOUSE_RELEASED && !gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_mousedownpos, mx, my, mw, mh))
            {
                nNewFocus = k;
                bClick = true;
            }
        }
    }
    nTopDelta += nNewFocus-nFocus;
    nFocus = nNewFocus;
    if (bClick)
    {
        CGameMenuEvent event = { kMenuEventEnter, 0 };

        gGameMenuMgr.m_mousecaught = 1;

        if (Event(event))
            gGameMenuMgr.PostPop();
    }
}

bool CGameMenuItemKeyList::Event(CGameMenuEvent &event)
{
    if (bScan)
    {
        if (KB_LastScan && KB_LastScan != sc_Pause)
        {
            if (KB_KeyWaiting())
                KB_GetCh();
            char key1, key2;
            extern uint8_t KeyboardKeys[NUMGAMEFUNCTIONS][2];
            key1 = KeyboardKeys[nFocus][0];
            key2 = KeyboardKeys[nFocus][1];
            if (key1 > 0 && key2 != KB_LastScan)
                key2 = key1;
            key1 = KB_LastScan;
            if (key1 == key2)
                key2 = 0;
            uint8_t oldKey[2];
            oldKey[0] = KeyboardKeys[nFocus][0];
            oldKey[1] = KeyboardKeys[nFocus][1];
            KeyboardKeys[nFocus][0] = key1;
            KeyboardKeys[nFocus][1] = key2;
            CONFIG_MapKey(nFocus, key1, oldKey[0], key2, oldKey[1]);
            KB_FlushKeyboardQueue();
            KB_FlushKeyboardQueueScans();
            KB_ClearKeysDown();
            keyFlushScans();
            keyFlushChars();
            bScan = 0;
        }
        return false;
    }
    switch (event.at0)
    {
    case kMenuEventUp:
        if (event.at2 == sc_Tab || nFocus == 0)
        {
            pMenu->FocusPrevItem();
            return false;
        }
        nFocus--;
        if (nTopDelta > 0)
            nTopDelta--;
        return false;
    case kMenuEventDown:
        if (event.at2 == sc_Tab || nFocus == nGameFuncs-1)
        {
            pMenu->FocusNextItem();
            return false;
        }
        nFocus++;
        if (nTopDelta+1 < nRows)
            nTopDelta++;
        return false;
    case kMenuEventEnter:
        if (pCallback)
            pCallback(this);
        Scan();
        return false;
    case kMenuEventDelete:
        if (keystatus[sc_LeftControl] || keystatus[sc_RightControl])
        {
            uint8_t oldKey[2];
            oldKey[0] = KeyboardKeys[nFocus][0];
            oldKey[1] = KeyboardKeys[nFocus][1];
            KeyboardKeys[nFocus][0] = 0;
            KeyboardKeys[nFocus][1] = 0;
            CONFIG_MapKey(nFocus, 0, oldKey[0], 0, oldKey[1]);
        }
        return false;
    case kMenuEventScrollUp:
        if (nFocus-nTopDelta > 0)
        {
            nTopDelta++;
            if (nTopDelta>0)
            {
                nFocus--;
                nTopDelta--;
            }
        }
        return false;
    case kMenuEventScrollDown:
        if (nFocus-nTopDelta+nRows < nGameFuncs)
        {
            nTopDelta--;
            if (nTopDelta+1 < nRows)
            {
                nFocus++;
                nTopDelta++;
            }
        }
        return false;
    }
    return CGameMenuItem::Event(event);
}

bool CGameMenuItemKeyList::MouseEvent(CGameMenuEvent &event)
{
    event.at0 = kMenuEventNone;
    if (MOUSEACTIVECONDITIONAL(MOUSE_GetButtons()&WHEELUP_MOUSE))
    {
        gGameMenuMgr.m_mouselastactivity = totalclock;
        MOUSE_ClearButton(WHEELUP_MOUSE);
        event.at0 = kMenuEventScrollUp;
    }
    else if (MOUSEACTIVECONDITIONAL(MOUSE_GetButtons()&WHEELDOWN_MOUSE))
    {
        gGameMenuMgr.m_mouselastactivity = totalclock;
        MOUSE_ClearButton(WHEELDOWN_MOUSE);
        event.at0 = kMenuEventScrollDown;
    }
    else
        return CGameMenuItem::MouseEvent(event);
    return event.at0 != kMenuEventNone;
}

CGameMenuItemSlider::CGameMenuItemSlider()
{
    m_pzText = NULL;
    m_nFont = -1;
    m_nX = 0;
    m_nY = 0;
    nValue = 0;
    nRangeLow = 0;
    nStep = 0;
    pCallback = NULL;
    pValue = NULL;
    nSliderTile = 2204;
    nCursorTile = 2028;
    nShowValue = kMenuSliderNone;
}

CGameMenuItemSlider::CGameMenuItemSlider(const char *_pzText, int _nFont, int _nX, int _nY, int _nWidth, int _nValue, int _nRangeLow, int _nRangeHigh, int _nStep, void(*_pCallback)(CGameMenuItemSlider *), int _nSliderTile, int _nCursorTile, int _nShowValue)
{
    m_pzText = _pzText;
    m_nFont = _nFont;
    m_nX = _nX;
    m_nY = _nY;
    m_nWidth = _nWidth;
    nRangeLow = _nRangeLow;
    nRangeHigh = _nRangeHigh;
    nStep = _nStep;
    nValue = ClipRange(_nValue, nRangeLow, nRangeHigh);
    pCallback = _pCallback;
    nSliderTile = 2204;
    nCursorTile = 2028;
    if (_nSliderTile >= 0)
        nSliderTile = _nSliderTile;
    if (_nCursorTile >= 0)
        nCursorTile = _nCursorTile;
    nShowValue = _nShowValue;
}

CGameMenuItemSlider::CGameMenuItemSlider(const char *_pzText, int _nFont, int _nX, int _nY, int _nWidth, int *pnValue, int _nRangeLow, int _nRangeHigh, int _nStep, void(*_pCallback)(CGameMenuItemSlider *), int _nSliderTile, int _nCursorTile, int _nShowValue)
{
    m_pzText = _pzText;
    m_nFont = _nFont;
    m_nX = _nX;
    m_nY = _nY;
    m_nWidth = _nWidth;
    nRangeLow = _nRangeLow;
    nRangeHigh = _nRangeHigh;
    nStep = _nStep;
    dassert(pnValue != NULL);
    pValue = pnValue;
    nValue = ClipRange(*pnValue, nRangeLow, nRangeHigh);
    pCallback = _pCallback;
    nSliderTile = 2204;
    nCursorTile = 2028;
    if (_nSliderTile >= 0)
        nSliderTile = _nSliderTile;
    if (_nCursorTile >= 0)
        nCursorTile = _nCursorTile;
    nShowValue = _nShowValue;
}

void CGameMenuItemSlider::Draw(void)
{
    char buffer[16];
    int height;
    nValue = pValue ? *pValue : nValue;
    gMenuTextMgr.GetFontInfo(m_nFont, NULL, NULL, &height);
    int shade = bEnable ? 32 : 48;
    int shade2 = bEnable ? 0 : 16;
    int pal = bEnable ? 0 : 5;
    if (pMenu->IsFocusItem(this))
        shade = 32-(totalclock&63);
    if (m_pzText)
        gMenuTextMgr.DrawText(m_pzText, m_nFont, m_nX, m_nY, shade, pal, false);
    int sliderX = m_nX+m_nWidth-1-tilesiz[nSliderTile].x/2;
    rotatesprite(sliderX<<16, (m_nY+height/2)<<16, 65536, 0, nSliderTile, shade2, pal, 10, 0, 0, xdim-1, ydim-1);
    int nRange = nRangeHigh - nRangeLow;
    dassert(nRange > 0);
    int value = nValue - nRangeLow;
    int width = tilesiz[nSliderTile].x-8;
    int cursorX = sliderX + ksgn(nStep)*(value * width / nRange - width / 2);
    rotatesprite(cursorX<<16, (m_nY+height/2)<<16, 65536, 0, nCursorTile, shade2, pal, 10, 0, 0, xdim-1, ydim-1);

    buffer[0] = 0;
    switch (nShowValue)
    {
    case kMenuSliderNone:
        break;
    case kMenuSliderValue:
        sprintf(buffer, "%i ", nValue);
        break;
    case kMenuSliderPercent:
        sprintf(buffer, "%i%% ", roundscale(value, 100, nRange));
        break;
    case kMenuSliderQ16:
        snprintf(buffer, 16, "%.3f ", nValue/65536.f);
        break;
    }
    int valueWidth;
    gMenuTextMgr.GetFontInfo(m_nFont, buffer, &valueWidth, NULL);
    int valueX = m_nX+m_nWidth-1-tilesiz[nSliderTile].x-valueWidth;
    gMenuTextMgr.DrawText(buffer, m_nFont, valueX, m_nY, 32, 0, false);

    int mx = m_nX;
    int my = m_nY;
    int mw = m_nWidth;
    int mh = height;
    if (height < tilesiz[nSliderTile].y)
    {
        my -= (tilesiz[nSliderTile].y-height)/2;
        height = tilesiz[nSliderTile].y;
    }
    mx <<= 16;
    my <<= 16;
    mw <<= 16;
    mh <<= 16;

    if (bEnable && MOUSEACTIVECONDITIONAL(!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_mousepos, mx, my, mw, mh)))
    {
        if (MOUSEWATCHPOINTCONDITIONAL(!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_prevmousepos, mx, my, mw, mh)))
        {
            pMenu->SetFocusItem(this);
        }

        if (!gGameMenuMgr.m_mousecaught && (g_mouseClickState == MOUSE_PRESSED || g_mouseClickState == MOUSE_HELD))
        {
            pMenu->SetFocusItem(this);

            int sliderx = m_nX+m_nWidth-1-tilesiz[nSliderTile].x;
            int sliderwidth = tilesiz[nSliderTile].x;
            int regionwidth = sliderwidth-8;
            int regionx = sliderx+(sliderwidth-regionwidth)/2;
            sliderx <<= 16;
            sliderwidth <<= 16;
            regionwidth <<= 16;
            regionx <<= 16;

            // region between the x-midline of the slidepoint at the extremes slides proportionally
            if (!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_mousepos, regionx, my, regionwidth, mh))
            {
                int dx = (gGameMenuMgr.m_mousepos.x - (regionx+regionwidth/2))*ksgn(nStep);
                nValue = nRangeLow + roundscale(dx+regionwidth/2, nRange, regionwidth);
                nValue = ClipRange(nValue, nRangeLow, nRangeHigh);
                if (pCallback)
                    pCallback(this);
                gGameMenuMgr.m_mousecaught = 1;
            }
            // region outside the x-midlines clamps to the extremes
            else if (!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_mousepos, sliderx, my, sliderwidth, mh))
            {
                if ((gGameMenuMgr.m_mousepos.x-(regionx+regionwidth/2))*ksgn(nStep) > 0)
                    nValue = nRangeHigh;
                else
                    nValue = nRangeLow;
                if (pCallback)
                    pCallback(this);
                gGameMenuMgr.m_mousecaught = 1;
            }
        }
    }
}

bool CGameMenuItemSlider::Event(CGameMenuEvent &event)
{
    nValue = pValue ? *pValue : nValue;
    switch (event.at0)
    {
    case kMenuEventUp:
        pMenu->FocusPrevItem();
        return false;
    case kMenuEventDown:
        pMenu->FocusNextItem();
        return false;
    case kMenuEventLeft:
        if (nStep > 0)
            nValue = DecBy(nValue, nStep);
        else
            nValue = IncBy(nValue, -nStep);
        nValue = ClipRange(nValue, nRangeLow, nRangeHigh);
        if (pCallback)
            pCallback(this);
        return false;
    case kMenuEventRight:
        if (nStep >= 0)
            nValue = IncBy(nValue, nStep);
        else
            nValue = DecBy(nValue, -nStep);
        nValue = ClipRange(nValue, nRangeLow, nRangeHigh);
        if (pCallback)
            pCallback(this);
        return false;
    case kMenuEventEnter:
        if (pCallback)
            pCallback(this);
        return false;
    }
    return CGameMenuItem::Event(event);
}

bool CGameMenuItemSlider::MouseEvent(CGameMenuEvent &event)
{
    event.at0 = kMenuEventNone;
    if (MOUSEINACTIVECONDITIONAL((MOUSE_GetButtons()&LEFT_MOUSE) && (MOUSE_GetButtons()&WHEELUP_MOUSE)))
    {
        MOUSE_ClearButton(WHEELUP_MOUSE);
        event.at0 = kMenuEventLeft;
    }
    else if (MOUSEINACTIVECONDITIONAL((MOUSE_GetButtons()&LEFT_MOUSE) && (MOUSE_GetButtons()&WHEELDOWN_MOUSE)))
    {
        MOUSE_ClearButton(WHEELDOWN_MOUSE);
        event.at0 = kMenuEventRight;
    }
    else if (MOUSE_GetButtons()&RIGHT_MOUSE)
    {
        MOUSE_ClearButton(RIGHT_MOUSE);
        event.at0 = kMenuEventEscape;
    }
    else if (MOUSE_GetButtons()&WHEELUP_MOUSE)
    {
        MOUSE_ClearButton(WHEELUP_MOUSE);
        MOUSE_ClearButton(LEFT_MOUSE);
        event.at0 = kMenuEventUp;
    }
    else if (MOUSE_GetButtons()&WHEELDOWN_MOUSE)
    {
        MOUSE_ClearButton(WHEELDOWN_MOUSE);
        MOUSE_ClearButton(LEFT_MOUSE);
        event.at0 = kMenuEventDown;
    }
    return event.at0 != kMenuEventNone;
}

CGameMenuItemSliderFloat::CGameMenuItemSliderFloat()
{
    m_pzText = NULL;
    m_nFont = -1;
    m_nX = 0;
    m_nY = 0;
    fValue = 0;
    fRangeLow = 0;
    fStep = 0;
    pCallback = NULL;
    pValue = NULL;
    nSliderTile = 2204;
    nCursorTile = 2028;
    nShowValue = kMenuSliderNone;
}

CGameMenuItemSliderFloat::CGameMenuItemSliderFloat(const char *_pzText, int _nFont, int _nX, int _nY, int _nWidth, float _fValue, float _fRangeLow, float _fRangeHigh, float _fStep, void(*_pCallback)(CGameMenuItemSliderFloat *), int _nSliderTile, int _nCursorTile, int _nShowValue)
{
    m_pzText = _pzText;
    m_nFont = _nFont;
    m_nX = _nX;
    m_nY = _nY;
    m_nWidth = _nWidth;
    fRangeLow = _fRangeLow;
    fRangeHigh = _fRangeHigh;
    fStep = _fStep;
    fValue = ClipRangeF(_fValue, fRangeLow, fRangeHigh);
    pCallback = _pCallback;
    nSliderTile = 2204;
    nCursorTile = 2028;
    if (_nSliderTile >= 0)
        nSliderTile = _nSliderTile;
    if (_nCursorTile >= 0)
        nCursorTile = _nCursorTile;
    nShowValue = _nShowValue;
}

CGameMenuItemSliderFloat::CGameMenuItemSliderFloat(const char *_pzText, int _nFont, int _nX, int _nY, int _nWidth, float *pnValue, float _fRangeLow, float _fRangeHigh, float _fStep, void(*_pCallback)(CGameMenuItemSliderFloat *), int _nSliderTile, int _nCursorTile, int _nShowValue)
{
    m_pzText = _pzText;
    m_nFont = _nFont;
    m_nX = _nX;
    m_nY = _nY;
    m_nWidth = _nWidth;
    fRangeLow = _fRangeLow;
    fRangeHigh = _fRangeHigh;
    fStep = _fStep;
    dassert(pnValue != NULL);
    pValue = pnValue;
    fValue = ClipRangeF(*pnValue, fRangeLow, fRangeHigh);
    pCallback = _pCallback;
    nSliderTile = 2204;
    nCursorTile = 2028;
    if (_nSliderTile >= 0)
        nSliderTile = _nSliderTile;
    if (_nCursorTile >= 0)
        nCursorTile = _nCursorTile;
    nShowValue = _nShowValue;
}

void CGameMenuItemSliderFloat::Draw(void)
{
    char buffer[16];
    int height;

    fValue = pValue ? *pValue : fValue;
    gMenuTextMgr.GetFontInfo(m_nFont, NULL, NULL, &height);
    int shade = bEnable ? 32 : 48;
    int shade2 = bEnable ? 0 : 16;
    int pal = bEnable ? 0 : 5;
    if (pMenu->IsFocusItem(this))
        shade = 32-(totalclock&63);
    if (m_pzText)
        gMenuTextMgr.DrawText(m_pzText, m_nFont, m_nX, m_nY, shade, pal, false);
    int sliderX = m_nX+m_nWidth-1-tilesiz[nSliderTile].x/2;
    rotatesprite(sliderX<<16, (m_nY+height/2)<<16, 65536, 0, nSliderTile, shade2, pal, 10, 0, 0, xdim-1, ydim-1);
    float fRange = fRangeHigh - fRangeLow;
    dassert(fRange > 0);
    float value = fValue - fRangeLow;
    int width = tilesiz[nSliderTile].x-8;
    int cursorX = sliderX + (int)(ksgnf(fStep)*(value * width / fRange - width / 2));
    rotatesprite(cursorX<<16, (m_nY+height/2)<<16, 65536, 0, nCursorTile, shade2, pal, 10, 0, 0, xdim-1, ydim-1);

    buffer[0] = 0;
    switch (nShowValue)
    {
    case kMenuSliderNone:
        break;
    case kMenuSliderValue:
        snprintf(buffer, 16, "%.3f ", fValue);
        break;
    case kMenuSliderPercent:
        snprintf(buffer, 16, "%.3f%% ", value*100.f/fRange);
        break;
    }
    int valueWidth;
    gMenuTextMgr.GetFontInfo(m_nFont, buffer, &valueWidth, NULL);
    int valueX = m_nX+m_nWidth-1-tilesiz[nSliderTile].x-valueWidth;
    gMenuTextMgr.DrawText(buffer, m_nFont, valueX, m_nY, 32, 0, false);

    int mx = m_nX;
    int my = m_nY;
    int mw = m_nWidth;
    int mh = height;
    if (height < tilesiz[nSliderTile].y)
    {
        my -= (tilesiz[nSliderTile].y-height)/2;
        height = tilesiz[nSliderTile].y;
    }
    mx <<= 16;
    my <<= 16;
    mw <<= 16;
    mh <<= 16;

    if (bEnable && MOUSEACTIVECONDITIONAL(!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_mousepos, mx, my, mw, mh)))
    {
        if (MOUSEWATCHPOINTCONDITIONAL(!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_prevmousepos, mx, my, mw, mh)))
        {
            pMenu->SetFocusItem(this);
        }

        if (!gGameMenuMgr.m_mousecaught && (g_mouseClickState == MOUSE_PRESSED || g_mouseClickState == MOUSE_HELD))
        {
            pMenu->SetFocusItem(this);

            int sliderx = m_nX+m_nWidth-1-tilesiz[nSliderTile].x;
            int sliderwidth = tilesiz[nSliderTile].x;
            int regionwidth = sliderwidth-8;
            int regionx = sliderx+(sliderwidth-regionwidth)/2;
            sliderx <<= 16;
            sliderwidth <<= 16;
            regionwidth <<= 16;
            regionx <<= 16;

            // region between the x-midline of the slidepoint at the extremes slides proportionally
            if (!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_mousepos, regionx, my, regionwidth, mh))
            {
                int dx = (gGameMenuMgr.m_mousepos.x - (regionx+regionwidth/2))*ksgnf(fStep);
                fValue = fRangeLow + (dx+regionwidth/2) * fRange / regionwidth;
                fValue = ClipRangeF(fValue, fRangeLow, fRangeHigh);
                if (pCallback)
                    pCallback(this);
                gGameMenuMgr.m_mousecaught = 1;
            }
            // region outside the x-midlines clamps to the extremes
            else if (!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_mousepos, sliderx, my, sliderwidth, mh))
            {
                if ((gGameMenuMgr.m_mousepos.x-(regionx+regionwidth/2))*ksgnf(fStep) > 0)
                    fValue = fRangeHigh;
                else
                    fValue = fRangeLow;
                if (pCallback)
                    pCallback(this);
                gGameMenuMgr.m_mousecaught = 1;
            }
        }
    }
}

bool CGameMenuItemSliderFloat::Event(CGameMenuEvent &event)
{
    fValue = pValue ? *pValue : fValue;
    switch (event.at0)
    {
    case kMenuEventUp:
        pMenu->FocusPrevItem();
        return false;
    case kMenuEventDown:
        pMenu->FocusNextItem();
        return false;
    case kMenuEventLeft:
        if (fStep > 0)
            fValue -= fStep;
        else
            fValue += fStep;
        fValue = ClipRangeF(fValue, fRangeLow, fRangeHigh);
        if (pCallback)
            pCallback(this);
        return false;
    case kMenuEventRight:
        if (fStep >= 0)
            fValue += fStep;
        else
            fValue -= fStep;
        fValue = ClipRangeF(fValue, fRangeLow, fRangeHigh);
        if (pCallback)
            pCallback(this);
        return false;
    case kMenuEventEnter:
        if (pCallback)
            pCallback(this);
        return false;
    }
    return CGameMenuItem::Event(event);
}

CGameMenuItemZEdit::CGameMenuItemZEdit()
{
    m_pzText = NULL;
    m_nFont = -1;
    m_nX = 0;
    m_nY = 0;
    at20 = NULL;
    at24 = 0;
    at32 = 0;
    at2c = 0;
    at30 = 0;
    at28 = 0;
    at31 = 1;
}

CGameMenuItemZEdit::CGameMenuItemZEdit(const char *a1, int a2, int a3, int a4, int a5, char *a6, int a7, char a8, void(*a9)(CGameMenuItemZEdit *, CGameMenuEvent *), int a10)
{
    at30 = 0;
    at31 = 1;
    m_pzText = a1;
    m_nFont = a2;
    m_nX = a3;
    m_nY = a4;
    m_nWidth = a5;
    at20 = a6;
    at24 = a7;
    at32 = a8;
    at2c = a9;
    at28 = a10;
}

void CGameMenuItemZEdit::AddChar(char ch)
{
    int i = strlen(at20);
    if (i + 1 < at24)
    {
        at20[i] = ch;
        at20[i + 1] = 0;
    }
}

void CGameMenuItemZEdit::BackChar(void)
{
    int i = strlen(at20);
    if (i > 0)
        at20[i - 1] = 0;
}

void CGameMenuItemZEdit::Draw(void)
{
    int height, width, textWidth = 0;
    gMenuTextMgr.GetFontInfo(m_nFont, NULL, &width, &height);
    if (at20)
        gMenuTextMgr.GetFontInfo(m_nFont, at20, &textWidth, NULL);
    int shade = bEnable ? 32 : 48;
    int pal = bEnable ? 0 : 5;
    if (pMenu->IsFocusItem(this))
        shade = 32-(totalclock&63);
    if (at30)
        shade = -128;
    if (m_pzText)
        gMenuTextMgr.DrawText(m_pzText, m_nFont, m_nX, m_nY, shade, pal, false);
    int x = m_nX+m_nWidth-1-textWidth;//(at24+1)*width;
    if (at20 && *at20)
    {
        int width;
        gMenuTextMgr.GetFontInfo(m_nFont, at20, &width, NULL);
        int shade2;
        if (at32)
        {
            if (at30)
                shade2 = -128;
            else
                shade2 = shade;
        }
        else
        {
            if (at30)
                shade2 = shade;
            else
                shade2 = 32;
        }
        gMenuTextMgr.DrawText(at20, m_nFont, x, m_nY, shade2, pal, false);
        x += width;
    }
    if (at30 && (gGameClock & 32))
        gMenuTextMgr.DrawText("_", m_nFont, x, m_nY, shade, 0, false);

    int mx = m_nX<<16;
    int my = m_nY<<16;
    int mw = m_nWidth<<16;
    int mh = height<<16;

    if (bEnable && MOUSEACTIVECONDITIONAL(!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_mousepos, mx, my, mw, mh)))
    {
        if (MOUSEWATCHPOINTCONDITIONAL(!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_prevmousepos, mx, my, mw, mh)))
        {
            pMenu->SetFocusItem(this);
        }

        if (!gGameMenuMgr.m_mousecaught && g_mouseClickState == MOUSE_RELEASED && !gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_mousedownpos, mx, my, mw, mh))
        {
            pMenu->SetFocusItem(this);

            CGameMenuEvent event = { kMenuEventEnter, 0 };

            gGameMenuMgr.m_mousecaught = 1;

            if (Event(event))
                gGameMenuMgr.PostPop();
        }
    }
}

bool CGameMenuItemZEdit::Event(CGameMenuEvent &event)
{
    static char buffer[256];
    // Hack
    if (event.at2 == sc_kpad_2 || event.at2 == sc_kpad_4 || event.at2 == sc_kpad_6 || event.at2 == sc_kpad_8)
        event.at0 = kMenuEventKey;
    switch (event.at0)
    {
    case kMenuEventEscape:
        if (at30)
        {
            strncpy(at20, buffer, at24);
            at20[at24-1] = 0;
            at30 = 0;
            return false;
        }
        return true;
    case kMenuEventEnter:
        if (!at31)
        {
            if (at2c)
                at2c(this, &event);
            return false;
        }
        if (at30)
        {
            if (at2c)
                at2c(this, &event);
            at30 = 0;
            return false;
        }
        strncpy(buffer, at20, at24);
        buffer[at24-1] = 0;
        at30 = 1;
        return false;
    case kMenuEventBackSpace:
        if (at30)
            BackChar();
        return false;
    case kMenuEventKey:
    case kMenuEventSpace:
    {
        char key;
        if (event.at2 < 128)
        {
            if (keystatus[sc_LeftShift] || keystatus[sc_RightShift])
                key = g_keyAsciiTableShift[event.at2];
            else
                key = g_keyAsciiTable[event.at2];
            if (at30 && (isalnum(key) || ispunct(key) || isspace(key)))
            {
                AddChar(key);
                return false;
            }
        }
        return CGameMenuItem::Event(event);
    }
    case kMenuEventUp:
        if (at30)
            return false;
        return CGameMenuItem::Event(event);
    case kMenuEventDown:
        if (at30)
            return false;
        return CGameMenuItem::Event(event);
    }
    return CGameMenuItem::Event(event);
}

CGameMenuItemZEditBitmap::CGameMenuItemZEditBitmap()
{
    m_pzText = NULL;
    m_nFont = -1;
    m_nX = 0;
    m_nY = 0;
    at20 = NULL;
    at24 = 0;
    at36 = 0;
    at30 = NULL;
    at2c = NULL;
    at34 = 0;
    at28 = 0;
    at37 = 0;
    at35 = 1;
}

CGameMenuItemZEditBitmap::CGameMenuItemZEditBitmap(char *a1, int a2, int a3, int a4, int a5, char *a6, int a7, char a8, void(*a9)(CGameMenuItemZEditBitmap *, CGameMenuEvent *), int a10)
{
    at2c = NULL;
    at34 = 0;
    at35 = 1;
    at37 = 0;
    m_pzText = a1;
    m_nFont = a2;
    m_nX = a3;
    m_nY = a4;
    m_nWidth = a5;
    at20 = a6;
    at24 = a7;
    at36 = a8;
    at30 = a9;
    at28 = a10;
}

void CGameMenuItemZEditBitmap::AddChar(char ch)
{
    int i = strlen(at20);
    if (i + 1 < at24)
    {
        at20[i] = ch;
        at20[i + 1] = 0;
    }
}

void CGameMenuItemZEditBitmap::BackChar(void)
{
    int i = strlen(at20);
    if (i > 0)
        at20[i - 1] = 0;
}

void CGameMenuItemZEditBitmap::Draw(void)
{
    int height, width;
    gMenuTextMgr.GetFontInfo(m_nFont, NULL, &width, &height);
    int shade = bEnable ? 32 : 48;
    int pal = bEnable ? 0 : 5;
    if (pMenu->IsFocusItem(this))
        shade = 32-(totalclock&63);
    at2c->at24 = -1;
    if (at34)
        shade = -128;
    if (m_pzText)
        gMenuTextMgr.DrawText(m_pzText, m_nFont, m_nX, m_nY, shade, pal, false);
    int x = m_nX+m_nWidth-1-(at24+1)*width;
    if (at20 && *at20)
    {
        int width;
        gMenuTextMgr.GetFontInfo(m_nFont, at20, &width, NULL);
        int shade2;
        if (at36)
        {
            if (at34)
                shade2 = -128;
            else
                shade2 = shade;
        }
        else
        {
            if (at34)
                shade2 = shade;
            else
                shade2 = 32;
        }
        gMenuTextMgr.DrawText(at20, m_nFont, x, m_nY, shade2, 0, false);
        x += width;
    }
    if (at34 && (gGameClock & 32))
        gMenuTextMgr.DrawText("_", m_nFont, x, m_nY, shade, pal, false);

    int mx = m_nX<<16;
    int my = m_nY<<16;
    int mw = m_nWidth<<16;
    int mh = height<<16;

    if (bEnable && MOUSEACTIVECONDITIONAL(!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_mousepos, mx, my, mw, mh)))
    {
        if (MOUSEWATCHPOINTCONDITIONAL(!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_prevmousepos, mx, my, mw, mh)))
        {
            pMenu->SetFocusItem(this);
        }

        if (!gGameMenuMgr.m_mousecaught && g_mouseClickState == MOUSE_RELEASED && !gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_mousedownpos, mx, my, mw, mh))
        {
            pMenu->SetFocusItem(this);

            CGameMenuEvent event = { kMenuEventEnter, 0 };

            gGameMenuMgr.m_mousecaught = 1;

            if (Event(event))
                gGameMenuMgr.PostPop();
        }
    }
}

bool CGameMenuItemZEditBitmap::Event(CGameMenuEvent &event)
{
    static char buffer[256];
    // Hack
    if (event.at2 == sc_kpad_2 || event.at2 == sc_kpad_4 || event.at2 == sc_kpad_6 || event.at2 == sc_kpad_8)
        event.at0 = kMenuEventKey;
    switch (event.at0)
    {
    case kMenuEventEscape:
        if (at34)
        {
            strncpy(at20, buffer, at24);
            at20[at24-1] = 0;
            at34 = 0;
            gSaveGameActive = false;
            return false;
        }
        gSaveGameActive = true;
        return true;
    case kMenuEventEnter:
        if (!at35)
        {
            if (at30)
                at30(this, &event);
            gSaveGameActive = false;
            return false;
        }
        if (at34)
        {
            if (at30)
                at30(this, &event);
            at34 = 0;
            gSaveGameActive = false;
            return false;
        }
        strncpy(buffer, at20, at24);
        if (at37)
            at20[0] = 0;
        buffer[at24-1] = 0;
        at34 = 1;
        return false;
    case kMenuEventBackSpace:
        if (at34)
            BackChar();
        return false;
    case kMenuEventKey:
    case kMenuEventSpace:
    {
        char key;
        if (event.at2 < 128)
        {
            if (keystatus[sc_LeftShift] || keystatus[sc_RightShift])
                key = g_keyAsciiTableShift[event.at2];
            else
                key = g_keyAsciiTable[event.at2];
            if (at30 && (isalnum(key) || ispunct(key) || isspace(key)))
            {
                AddChar(key);
                return false;
            }
        }
        return CGameMenuItem::Event(event);
    }
    case kMenuEventUp:
        if (at34)
            return false;
        return CGameMenuItem::Event(event);
    case kMenuEventDown:
        if (at34)
            return false;
        return CGameMenuItem::Event(event);
    }
    return CGameMenuItem::Event(event);
}

CGameMenuItemQAV::CGameMenuItemQAV()
{
    at20 = NULL;
    at24 = NULL;
    at28 = 0;
    bEnable = 0;
}

CGameMenuItemQAV::CGameMenuItemQAV(const char *a1, int a2, int a3, int a4, const char *a5, bool widescreen, bool clearbackground)
{
    m_nWidth = 0;
    m_pzText = a1;
    m_nFont = a2;
    m_nY = a4;
    at20 = a5;
    m_nX = a3;
    bEnable = 0;
    bWideScreen = widescreen;
    bClearBackground = clearbackground;
}

void CGameMenuItemQAV::Draw(void)
{
    if (bClearBackground)
        videoClearScreen(0);
    if (at24)
    {
        int backFC = gFrameClock;
        gFrameClock = gGameClock;
        int nTicks = totalclock - at30;
        at30 = totalclock;
        at2c -= nTicks;
        if (at2c <= 0 || at2c > at28->at10)
        {
            at2c = at28->at10;
        }
        at28->Play(at28->at10 - at2c - nTicks, at28->at10 - at2c, -1, NULL);
        int wx1, wy1, wx2, wy2;
        wx1 = windowxy1.x;
        wy1 = windowxy1.y;
        wx2 = windowxy2.x;
        wy2 = windowxy2.y;
        windowxy1.x = 0;
        windowxy1.y = 0;
        windowxy2.x = xdim-1;
        windowxy2.y = ydim-1;
        if (bWideScreen)
        {
            int xdim43 = scale(ydim, 4, 3);
            int nCount = (xdim+xdim43-1)/xdim43;
            int backX = at28->x;
            for (int i = 0; i < nCount; i++)
            {
                at28->Draw(at28->at10 - at2c, 10+kQavOrientationLeft, 0, 0);
                at28->x += 320;
            }
            at28->x = backX;
        }
        else
            at28->Draw(at28->at10 - at2c, 10, 0, 0);

        windowxy1.x = wx1;
        windowxy1.y = wy1;
        windowxy2.x = wx2;
        windowxy2.y = wy2;
        gFrameClock = backFC;
    }

    if (bEnable && !gGameMenuMgr.m_mousecaught && g_mouseClickState == MOUSE_RELEASED)
    {
        pMenu->SetFocusItem(this);

        CGameMenuEvent event = { kMenuEventEnter, 0 };

        gGameMenuMgr.m_mousecaught = 1;

        if (Event(event))
            gGameMenuMgr.PostPop();
    }
}

bool CGameMenuItemQAV::Event(CGameMenuEvent &event)
{
    switch (event.at0)
    {
    case kMenuEventLeft:
    case kMenuEventBackSpace:
        pMenu->FocusPrevItem();
        return false;
    case kMenuEventRight:
    case kMenuEventEnter:
    case kMenuEventSpace:
        pMenu->FocusNextItem();
        return false;
    case kMenuEventInit:
        if (at20)
        {
            if (!at28)
            {
                at24 = gSysRes.Lookup(at20, "QAV");
                if (!at24)
                    ThrowError("Could not load QAV %s\n", at20);
                at28 = (QAV*)gSysRes.Lock(at24);
                at28->nSprite = -1;
                at28->x = m_nX;
                at28->y = m_nY;
                at28->Preload();
                at2c = at28->at10;
                at30 = totalclock;
                return false;
            }
            gSysRes.Lock(at24);
        }
        return false;
    case kMenuEventDeInit:
        if (at20 && at28)
        {
            gSysRes.Unlock(at24);
            if (at24->lockCount == 0)
                at28 = NULL;
        }
        return false;
    }
    return CGameMenuItem::Event(event);
}

void CGameMenuItemQAV::Reset(void)
{
    at2c = at28->at10;
    at30 = totalclock;
}

CGameMenuItemZCycleSelect::CGameMenuItemZCycleSelect()
{
    m_pzText = NULL;
    m_nFont = 3;
    m_nX = 0;
    m_nY = 0;
    m_nRows = 0;
    m_nTopDelta = 0;
    m_nFocus = 0;
    m_nItems = 0;
    m_pzStrings = NULL;
    m_pReturn = NULL;
}

CGameMenuItemZCycleSelect::CGameMenuItemZCycleSelect(const char *pzText, int nFont, int nX, int nY, int nWidth, int nRows, int nItems, const char **pzStrings, int *pReturn, void(*pCallback)(CGameMenuItemZCycleSelect *))
{
    m_nTopDelta = 0;
    m_nFocus = 0;
    m_pzText = pzText;
    m_nFont = nFont;
    m_nX = nX;
    m_nY = nY;
    m_nWidth = nWidth;
    m_nRows = nRows;
    m_pCallback = pCallback;
    m_nItems = nItems;
    m_pzStrings = pzStrings;
    m_pReturn = pReturn;
}

void CGameMenuItemZCycleSelect::Draw(void)
{
    int height;
    int shade;
    gMenuTextMgr.GetFontInfo(m_nFont, NULL, NULL, &height);
    int y = m_nY;
    int k = m_nFocus - m_nTopDelta;
    int nNewFocus = m_nFocus;
    bool bClick = false;
    for (int i = 0; i < m_nRows; i++, y += height, k++)
    {
        if (k == m_nFocus)
        {
            shade = 32;
            if (pMenu->IsFocusItem(this))
                shade = 32-(totalclock&63);
            viewDrawText(3, m_pzStrings[k], m_nX, y, shade, 0, 0, false);
        }
        else
        {
            viewDrawText(3, m_pzStrings[k], m_nX, y, 24, 0, 0, false);
        }
        int mx = m_nX<<16;
        int my = y<<16;
        int mw = m_nWidth<<16;
        int mh = height<<16;
        if (bEnable && MOUSEACTIVECONDITIONAL(!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_mousepos, mx, my, mw, mh)))
        {
            if (MOUSEWATCHPOINTCONDITIONAL(!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_prevmousepos, mx, my, mw, mh)))
            {
                nNewFocus = k;
            }

            if (!gGameMenuMgr.m_mousecaught && g_mouseClickState == MOUSE_RELEASED && !gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_mousedownpos, mx, my, mw, mh))
            {
                nNewFocus = k;
                bClick = true;
            }
        }
    }
    m_nTopDelta += nNewFocus-m_nFocus;
    m_nFocus = nNewFocus;
    if (bClick)
    {
        CGameMenuEvent event = { kMenuEventEnter, 0 };

        gGameMenuMgr.m_mousecaught = 1;

        if (Event(event))
            gGameMenuMgr.PostPop();
    }
}

bool CGameMenuItemZCycleSelect::Event(CGameMenuEvent &event)
{
    switch (event.at0)
    {
    case kMenuEventUp:
        if (event.at2 == sc_Tab || m_nFocus == 0)
        {
            pMenu->FocusPrevItem();
            return false;
        }
        m_nFocus--;
        if (m_nTopDelta > 0)
            m_nTopDelta--;
        return false;
    case kMenuEventDown:
        if (event.at2 == sc_Tab || m_nFocus == m_nItems-1)
        {
            pMenu->FocusNextItem();
            return false;
        }
        m_nFocus++;
        if (m_nTopDelta+1 < m_nRows)
            m_nTopDelta++;
        return false;
    case kMenuEventEnter:
        if (m_pCallback)
            m_pCallback(this);
        *m_pReturn = m_nFocus;
        return true;
    case kMenuEventScrollUp:
        if (m_nFocus-m_nTopDelta > 0)
        {
            m_nTopDelta++;
            if (m_nTopDelta>0)
            {
                m_nFocus--;
                m_nTopDelta--;
            }
        }
        return false;
    case kMenuEventScrollDown:
        if (m_nFocus-m_nTopDelta+m_nRows < m_nItems)
        {
            m_nTopDelta--;
            if (m_nTopDelta+1 < m_nRows)
            {
                m_nFocus++;
                m_nTopDelta++;
            }
        }
        return false;
    }
    return CGameMenuItem::Event(event);
}

bool CGameMenuItemZCycleSelect::MouseEvent(CGameMenuEvent &event)
{
    event.at0 = kMenuEventNone;
    if (MOUSEACTIVECONDITIONAL(MOUSE_GetButtons()&WHEELUP_MOUSE))
    {
        gGameMenuMgr.m_mouselastactivity = totalclock;
        MOUSE_ClearButton(WHEELUP_MOUSE);
        event.at0 = kMenuEventScrollUp;
    }
    else if (MOUSEACTIVECONDITIONAL(MOUSE_GetButtons()&WHEELDOWN_MOUSE))
    {
        gGameMenuMgr.m_mouselastactivity = totalclock;
        MOUSE_ClearButton(WHEELDOWN_MOUSE);
        event.at0 = kMenuEventScrollDown;
    }
    else
        return CGameMenuItem::MouseEvent(event);
    return event.at0 != kMenuEventNone;
}

CGameMenuItemZCycle::CGameMenuItemZCycle()
{
    m_pzText = NULL;
    m_nFocus = 0;
    m_nItems = 0;
    m_pCallback = NULL;
    m_pCallbackSelect = NULL;
    m_pMenuSelect = NULL;
    m_pItemSelectTitle = NULL;
    m_pItemSelect = NULL;
    m_nMenuSelectReturn = -1;
}

CGameMenuItemZCycle::CGameMenuItemZCycle(const char *a1, int a2, int a3, int a4, int a5, int a6, void(*a7)(CGameMenuItemZCycle *), const char **a8, int a9, int a10, bool bMenu, void(*pCallbackSelect)(CGameMenuItemZCycleSelect*))
{
    m_pzText = a1;
    m_nFont = a2;
    m_nX = a3;
    m_nY = a4;
    m_nFocus = 0;
    m_nWidth = a5;
    m_nAlign = a6;
    m_pCallback = a7;
    m_pCallbackSelect = pCallbackSelect;
    m_nItems = 0;
    m_bMenu = bMenu;
    m_pMenuSelect = NULL;
    m_pItemSelectTitle = NULL;
    m_pItemSelect = NULL;
    m_nMenuSelectReturn = -1;
    SetTextArray(a8, a9, a10);
}

CGameMenuItemZCycle::~CGameMenuItemZCycle()
{
    m_pzText = NULL;
    m_nFocus = 0;
    m_nItems = 0;
    m_pCallback = NULL;
    m_pCallbackSelect = NULL;
    m_pMenuSelect = NULL;
    m_pItemSelectTitle = NULL;
    m_pItemSelect = NULL;
    m_nMenuSelectReturn = -1;
    memset(m_pzStrings, 0, sizeof(m_pzStrings));
}

void CGameMenuItemZCycle::Draw(void)
{
    int width = 0, height = 0;
    int shade = bEnable ? 32 : 48;
    int pal = bEnable ? 0 : 5;
    if (pMenu->IsFocusItem(this))
        shade = 32-(totalclock&63);
    int x = m_nX;
    int y = m_nY;

    if (m_nMenuSelectReturn != -1)
    {
        m_nFocus = m_nMenuSelectReturn;
        if (m_pCallback)
            m_pCallback(this);
        m_nMenuSelectReturn = -1;
    }

    if (m_pzText)
    {
        gMenuTextMgr.GetFontInfo(m_nFont, m_pzText, &width, &height);
        switch (m_nAlign)
        {
        case 1:
            x = m_nX+m_nWidth/2-width/2;
            break;
        case 2:
            x = m_nX+m_nWidth-1-width;
            break;
        case 0:
        default:
            break;
        }
        gMenuTextMgr.DrawText(m_pzText, m_nFont, x, y, shade, pal, false);
    }
    const char *pzText;
    if (!m_nItems)
        pzText = "????";
    else
        pzText = m_pzStrings[m_nFocus];
    dassert(pzText != NULL);
    gMenuTextMgr.GetFontInfo(m_nFont, pzText, &width, NULL);
    gMenuTextMgr.DrawText(pzText, m_nFont, m_nX + m_nWidth - 1 - width, y, shade, pal, false);
    if (bEnable && MOUSEACTIVECONDITIONAL(!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_mousepos, x<<16, y<<16, m_nWidth<<16, height<<16)))
    {
        if (MOUSEWATCHPOINTCONDITIONAL(!gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_prevmousepos, x<<16, y<<16, m_nWidth<<16, height<<16)))
        {
            pMenu->SetFocusItem(this);
        }

        if (!gGameMenuMgr.m_mousecaught && g_mouseClickState == MOUSE_RELEASED && !gGameMenuMgr.MouseOutsideBounds(&gGameMenuMgr.m_mousedownpos, x<<16, y<<16, m_nWidth<<16, height<<16))
        {
            pMenu->SetFocusItem(this);

            CGameMenuEvent event = { kMenuEventEnter, 0 };

            gGameMenuMgr.m_mousecaught = 1;

            if (Event(event))
                gGameMenuMgr.PostPop();
        }
    }
}

bool CGameMenuItemZCycle::Event(CGameMenuEvent &event)
{
    switch (event.at0)
    {
    case kMenuEventEnter:
        if (m_bMenu)
        {
            if (m_pMenuSelect)
            {
                delete m_pMenuSelect;
                m_pMenuSelect = NULL;
            }
            if (m_pItemSelectTitle)
            {
                delete m_pItemSelectTitle;
                m_pItemSelectTitle = NULL;
            }
            if (m_pItemSelect)
            {
                delete m_pItemSelect;
                m_pItemSelect = NULL;
            }
            m_pMenuSelect = new CGameMenu();
            dassert(m_pMenuSelect != NULL);
            strncpy(m_zTitle, m_pzText, kMaxTitleLength);
            int l = strlen(m_zTitle);
            if (l > 0 && m_zTitle[l-1] == ':')
                l--;
            m_zTitle[l] = 0;
            m_pItemSelectTitle = new CGameMenuItemTitle(m_zTitle, 1, 160, 20, 2038);
            dassert(m_pItemSelectTitle != NULL);
            m_pItemSelect = new CGameMenuItemZCycleSelect("", 3, 100, 40, 100, 16, m_nItems, m_pzStrings, &m_nMenuSelectReturn, m_pCallbackSelect);
            dassert(m_pItemSelect != NULL);
            m_pMenuSelect->Add(m_pItemSelectTitle, false);
            m_pMenuSelect->Add(m_pItemSelect, true);
            m_pMenuSelect->Add(&itemBloodQAV, false);
            gGameMenuMgr.Push(m_pMenuSelect, -1);
            return false;
        }
        fallthrough__;
    case kMenuEventRight:
    case kMenuEventSpace:
        Next();
        if (m_pCallback)
            m_pCallback(this);
        return false;
    case kMenuEventLeft:
        Prev();
        if (m_pCallback)
            m_pCallback(this);
        return false;
    case kMenuEventDeInit:
        if (m_pMenuSelect)
        {
            delete m_pMenuSelect;
            m_pMenuSelect = NULL;
        }
        if (m_pItemSelectTitle)
        {
            delete m_pItemSelectTitle;
            m_pItemSelectTitle = NULL;
        }
        if (m_pItemSelect)
        {
            delete m_pItemSelect;
            m_pItemSelect = NULL;
        }
        return false;
    }
    return CGameMenuItem::Event(event);
}

void CGameMenuItemZCycle::Add(const char *pItem, bool active)
{
    dassert(pItem != NULL);
    dassert(m_nItems < kMaxGameCycleItems);
    m_pzStrings[m_nItems] = pItem;
    if (active)
        m_nFocus = m_nItems;
    m_nItems++;
}

void CGameMenuItemZCycle::Next(void)
{
    if (m_nItems > 0)
    {
        m_nFocus++;
        if (m_nFocus >= m_nItems)
            m_nFocus = 0;
    }
}

void CGameMenuItemZCycle::Prev(void)
{
    if (m_nItems > 0)
    {
        m_nFocus--;
        if (m_nFocus < 0)
            m_nFocus += m_nItems;
    }
}

void CGameMenuItemZCycle::Clear(void)
{
    m_nItems = m_nFocus = 0;
    memset(m_pzStrings, 0, sizeof(m_pzStrings));
}

void CGameMenuItemZCycle::SetTextArray(const char **pTextArray, int nTextPtrCount, int nIndex)
{
    Clear();
    dassert(nTextPtrCount <= kMaxGameCycleItems);
    for (int i = 0; i < nTextPtrCount; i++)
        Add(pTextArray[i], false);
    SetTextIndex(nIndex);
}

void CGameMenuItemZCycle::SetTextIndex(int nIndex)
{
    m_nFocus = ClipRange(nIndex, 0, m_nItems);
}

CGameMenuItemYesNoQuit::CGameMenuItemYesNoQuit()
{
    m_pzText = NULL;
    m_nRestart = 0;
}

CGameMenuItemYesNoQuit::CGameMenuItemYesNoQuit(const char *a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
    m_pzText = a1;
    m_nFont = a2;
    m_nX = a3;
    m_nY = a4;
    m_nWidth = a5;
    at20 = a6;
    m_nRestart = a7;
}

void CGameMenuItemYesNoQuit::Draw(void)
{
    if (!m_pzText) return;
    int shade = 32;
    if (pMenu->IsFocusItem(this))
        shade = 32-(totalclock&63);
    int width;
    int x = m_nX;
    switch (at20)
    {
    case 1:
        gMenuTextMgr.GetFontInfo(m_nFont, m_pzText, &width, NULL);
        x = m_nX+m_nWidth/2-width/2;
        break;
    case 2:
        gMenuTextMgr.GetFontInfo(m_nFont, m_pzText, &width, NULL);
        x = m_nX+m_nWidth-1-width;
        break;
    case 0:
    default:
        break;
    }
    gMenuTextMgr.DrawText(m_pzText, m_nFont, x, m_nY, shade, 0, true);

    if (bEnable && !gGameMenuMgr.m_mousecaught && g_mouseClickState == MOUSE_RELEASED)
    {
        pMenu->SetFocusItem(this);

        CGameMenuEvent event = { kMenuEventEnter, 0 };

        gGameMenuMgr.m_mousecaught = 1;

        if (Event(event))
            gGameMenuMgr.PostPop();
    }
}

extern void Restart(CGameMenuItemChain *pItem);
extern void Quit(CGameMenuItemChain *pItem);

bool CGameMenuItemYesNoQuit::Event(CGameMenuEvent &event)
{
    switch (event.at0)
    {
    case kMenuEventKey:
        if (event.at2 == sc_Y)
        {
            if (m_nRestart)
                Restart(NULL);
            else
                Quit(NULL);
        }
        else if (event.at2 == sc_N)
            gGameMenuMgr.Pop();
        return false;
    case kMenuEventEnter:
        if (m_nRestart)
            Restart(NULL);
        else
            Quit(NULL);
        return false;
    }
    return CGameMenuItem::Event(event);
}

CGameMenuItemPicCycle::CGameMenuItemPicCycle()
{
    m_pzText = NULL;
    at24 = 0;
    m_nItems = 0;
    atb0 = 0;
    at2c = 0;
    atb4 = 0;
}

CGameMenuItemPicCycle::CGameMenuItemPicCycle(int a1, int a2, void(*a3)(CGameMenuItemPicCycle *), int *a4, int a5, int a6)
{
    m_nWidth = 0;
    at24 = 0;
    m_nItems = 0;
    m_nX = a1;
    m_nY = a2;
    atb0 = a3;
    atb4 = 0;
    SetPicArray(a4, a5, a6);
}

void CGameMenuItemPicCycle::Draw(void)
{
    videoSetViewableArea(0, 0, xdim - 1, ydim - 1);
    if (atb4)
        rotatesprite(0, 0, 65536, 0, atb4, 0, 0, 82, 0, 0, xdim - 1, ydim - 1);
    if (at30[at24])
        rotatesprite(0, 0, 65536, 0, at30[at24], 0, 0, 82, 0, 0, xdim - 1, ydim - 1);
}

bool CGameMenuItemPicCycle::Event(CGameMenuEvent &event)
{
    switch (event.at0)
    {
    case kMenuEventRight:
    case kMenuEventEnter:
    case kMenuEventSpace:
        Next();
        if (atb0)
            atb0(this);
        return false;
    case kMenuEventLeft:
        Prev();
        if (atb0)
            atb0(this);
        return false;
    }
    return CGameMenuItem::Event(event);
}

void CGameMenuItemPicCycle::Add(int nItem, bool active)
{
    dassert(m_nItems < kMaxPicCycleItems);
    at30[m_nItems] = nItem;
    if (active)
        at24 = m_nItems;
    m_nItems++;
}

void CGameMenuItemPicCycle::Next(void)
{
    if (m_nItems > 0)
    {
        at24++;
        if (at24 >= m_nItems)
            at24 = 0;
    }
}

void CGameMenuItemPicCycle::Prev(void)
{
    if (m_nItems > 0)
    {
        at24--;
        if (at24 < 0)
            at24 += m_nItems;
    }
}

void CGameMenuItemPicCycle::Clear(void)
{
    m_nItems = at24 = 0;
    memset(at30, 0, sizeof(at30));
    at2c = 0;
}

void CGameMenuItemPicCycle::SetPicArray(int *pArray, int nTileCount, int nIndex)
{
    Clear();
    at2c = 0;
    dassert(nTileCount <= kMaxPicCycleItems);
    for (int i = 0; i < nTileCount; i++)
        Add(pArray[i], false);
    SetPicIndex(nIndex);
}

void CGameMenuItemPicCycle::SetPicIndex(int nIndex)
{
    at24 = ClipRange(nIndex, 0, m_nItems);
}

CGameMenuItemPassword::CGameMenuItemPassword()
{
    at37 = 0;
    m_pzText = NULL;
    at36 = 0;
    at32 = 0;
    at5b = 0;
}

CGameMenuItemPassword::CGameMenuItemPassword(const char *a1, int a2, int a3, int a4)
{
    at37 = 0;
    m_nWidth = 0;
    at36 = 0;
    at32 = 0;
    at5b = 0;
    m_pzText = a1;
    m_nFont = a2;
    m_nX = a3;
    m_nY = a4;
}

const char *kCheckPasswordMsg = "ENTER PASSWORD: ";
const char *kOldPasswordMsg = "ENTER OLD PASSWORD: ";
const char *kNewPasswordMsg = "ENTER NEW PASSWORD: ";
const char *kInvalidPasswordMsg = "INVALID PASSWORD.";

void CGameMenuItemPassword::Draw(void)
{
    bool focus = pMenu->IsFocusItem(this);
    int shade = 32;
    int shadef = 32-(totalclock&63);
    int width;
    switch (at37)
    {
    case 1:
    case 2:
    case 3:
        switch (at37)
        {
        case 1:
            strcpy(at3b, kCheckPasswordMsg);
            break;
        case 2:
            strcpy(at3b, kOldPasswordMsg);
            break;
        case 3:
            strcpy(at3b, kNewPasswordMsg);
            break;
        }
        for (int i = 0; i < at32; i++)
            strcat(at3b, "*");
        strcat(at3b, "_");
        gMenuTextMgr.GetFontInfo(m_nFont, at3b, &width, NULL);
        gMenuTextMgr.DrawText(at3b, m_nFont, m_nX-width/2, m_nY+20, shadef, 0, false);
        shadef = 32;
        break;
    case 4:
        if ((totalclock - at5b) & 32)
        {
            gMenuTextMgr.GetFontInfo(m_nFont, kInvalidPasswordMsg, &width, NULL);
            gMenuTextMgr.DrawText(kInvalidPasswordMsg, m_nFont, m_nX - width / 2, m_nY + 20, shade, 0, false);
        }
        if (at5b && totalclock-at5b > 256)
        {
            at5b = 0;
            at37 = 0;
        }
        break;
    }
    gMenuTextMgr.GetFontInfo(m_nFont, m_pzText, &width, NULL);
    gMenuTextMgr.DrawText(m_pzText, m_nFont, m_nX-width/2, m_nY, focus ? shadef : shade, 0, false);
}

bool CGameMenuItemPassword::Event(CGameMenuEvent &event)
{
    switch (at37)
    {
    case 0:
    case 4:
        if (event.at0 == kMenuEventEnter)
        {
            at29[0] = 0;
            if (strcmp(at20, ""))
                at37 = 2;
            else
                at37 = 3;
            return false;
        }
        return CGameMenuItem::Event(event);
    case 1:
    case 2:
    case 3:
        switch (event.at0)
        {
        case kMenuEventEnter:
            switch (at37)
            {
            case 1:
                at36 = strcmp(at20,at29) == 0;
                if (at36)
                    at37 = 0;
                else
                    at37 = 4;
                if (!at36)
                {
                    at5b = totalclock;
                    pMenu->FocusPrevItem();
                }
                else
                {
                    at5f->at20 = 0;
                    at5f->Draw();
                    gbAdultContent = false;
                    // NUKE-TODO:
                    //CONFIG_WriteAdultMode();
                    pMenu->FocusPrevItem();
                }
                return false;
            case 2:
                at36 = strcmp(at20,at29) == 0;
                if (at36)
                    at37 = 0;
                else
                    at37 = 4;
                if (at36)
                {
                    strcpy(at20, "");
                    strcpy(gzAdultPassword, "");
                    // NUKE-TODO:
                    //CONFIG_WriteAdultMode();
                    at37 = 0;
                }
                else
                    at5b = totalclock;
                return false;
            case 3:
                strcpy(at20, at29);
                strcpy(at20, gzAdultPassword);
                strcpy(gzAdultPassword, "");
                // NUKE-TODO:
                //CONFIG_WriteAdultMode();
                at37 = 0;
                return false;
            }
            break;
        case kMenuEventEscape:
            at37 = 0;
            Draw();
            return false;
        case kMenuEventKey:
            if (at32 < 8)
            {
                char key = Btoupper(g_keyAsciiTable[event.at2]);
                if (isalnum(key) || ispunct(key) || isspace(key))
                {
                    at29[at32++] = key;
                    at29[at32] = 0;
                }
            }
            return false;
        case kMenuEventBackSpace:
            if (at32 > 0)
                at29[--at32] = 0;
            return false;
        case kMenuEventLeft:
        case kMenuEventRight:
        case kMenuEventSpace:
            return false;
        }
    }
    return CGameMenuItem::Event(event);
}