//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT
Copyright (C) 2020 Christoph Oelckers

This file is part of Raze.

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

#include "ns.h"	// Must come before everything else!

#include "build.h"
#include "compat.h"
#include "c_bind.h"
#include "razemenu.h"
#include "gamestate.h"
#include "v_video.h"
#include "v_draw.h"
#include "vm.h"
#include "blood.h"

bool ShowOptionMenu();

BEGIN_BLD_NS

class CGameMenuItemQAV
{
public:
	int m_nX, m_nY;
	TArray<uint8_t> raw;
	int duration;
	int lastTick;
	bool bWideScreen;
	bool bClearBackground;
	const char* filename;
	CGameMenuItemQAV(int, int, const char*, bool widescreen = false, bool clearbackground = false);
	void Draw(void);
};

CGameMenuItemQAV::CGameMenuItemQAV(int a3, int a4, const char* name, bool widescreen, bool clearbackground)
{
	m_nY = a4;
	m_nX = a3;
	bWideScreen = widescreen;
	bClearBackground = clearbackground;
	filename = name;

	if (name)
	{
		// NBlood read this directly from the file system cache, but let's better store the data locally for robustness.
		raw = fileSystem.LoadFile(name, 0);
		if (raw.Size() != 0)
		{
			auto data = (QAV*)raw.Data();
			data->nSprite = -1;
			data->x = m_nX;
			data->y = m_nY;
			//data->Preload();
			duration = data->duration;
			lastTick = I_GetTime();
		}
	}
}

void CGameMenuItemQAV::Draw(void)
{
	if (bClearBackground)
		twod->ClearScreen();

	if (raw.Size() > 0)
	{
		auto data = (QAV*)raw.Data();

		auto thisTick = I_GetTime();
		if (!lastTick)
		{
			lastTick = thisTick;
		}
		if (lastTick < thisTick)
		{
			lastTick = thisTick;
			duration -= 4;
		}

		if (duration <= 0 || duration > data->duration)
		{
			duration = data->duration;
		}
		auto currentDuration = data->duration - duration;
		auto smoothratio = I_GetTimeFrac() * MaxSmoothRatio;

		data->Play(currentDuration - 4, currentDuration, -1, NULL);

		if (bWideScreen)
		{
			int xdim43 = scale(ydim, 4, 3);
			int nCount = (twod->GetWidth() + xdim43 - 1) / xdim43;
			int backX = data->x;
			for (int i = 0; i < nCount; i++)
			{
				data->Draw(currentDuration, 10 + kQavOrientationLeft, 0, 0, false, smoothratio, filename == "BDRIP.QAV");
				data->x += 320;
			}
			data->x = backX;
		}
		else
			data->Draw(currentDuration, 10, 0, 0, false, smoothratio, filename == "BDRIP.QAV");
	}
}


static std::unique_ptr<CGameMenuItemQAV> itemBloodQAV;	// This must be global to ensure that the animation remains consistent across menus.

void UpdateNetworkMenus(void)
{
	// For now disable the network menu item as it is not functional.
	for (auto name : { NAME_Mainmenu, NAME_IngameMenu })
	{
		DMenuDescriptor** desc = MenuDescriptors.CheckKey(name);
		if (desc != NULL && (*desc)->IsKindOf(RUNTIME_CLASS(DListMenuDescriptor)))
		{
			DListMenuDescriptor* ld = static_cast<DListMenuDescriptor*>(*desc);
			for (auto& li : ld->mItems)
			{
				if (li->mAction == NAME_MultiMenu)
				{
					li->mEnabled = -1;
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
//
// Menu related game interface functions
//
//----------------------------------------------------------------------------

void GameInterface::MenuOpened()
{
	itemBloodQAV.reset(new CGameMenuItemQAV(160, 100, "BDRIP.QAV", true));
}

void GameInterface::MenuClosed()
{
	itemBloodQAV.reset();
}

bool GameInterface::CanSave()
{
	return (gamestate == GS_LEVEL && gPlayer[myconnectindex].pXSprite->health != 0);
}

FSavegameInfo GameInterface::GetSaveSig()
{
	return { SAVESIG_BLD, MINSAVEVER_BLD, SAVEVER_BLD };
}

END_BLD_NS

using namespace Blood;
//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(DListMenuItemBloodDripDrawer, Draw)
{
	// For narrow screens this would be mispositioned so skip drawing it there.
	double ratio = ActiveRatio(screen->GetWidth(), screen->GetHeight());
	if (ratio > 1.32) itemBloodQAV->Draw();
	return 0;
}

DEFINE_ACTION_FUNCTION(_ImageScrollerPageQavDrawer, LoadQav)
{
	PARAM_PROLOGUE;
	PARAM_STRING(str);
	auto qav = new CGameMenuItemQAV(160, 100, str, false, true);
	ACTION_RETURN_POINTER(qav);
}

DEFINE_ACTION_FUNCTION(_ImageScrollerPageQavDrawer, DestroyQav)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(qav, CGameMenuItemQAV);
	if (qav) delete qav;
	return 0;
}

DEFINE_ACTION_FUNCTION(_ImageScrollerPageQavDrawer, DrawQav)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(qav, CGameMenuItemQAV);
	qav->Draw();
	return 0;
}


