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
#pragma once
#include "build.h"
#include "common_game.h"

BEGIN_BLD_NS

class CKillMgr {
public:
	int TotalKills, Kills;
	void SetCount(int);
	void AddNewKill(int);
	void AddKill(DBloodActor* actor);
	void CountTotalKills(void);
	void Clear(void);
};

class CSecretMgr {
public:
	int Total, Founds, Super;
	void SetCount(int);
	void Found(int);
	void Clear(void);
};

extern CSecretMgr gSecretMgr;
extern CKillMgr gKillMgr;


END_BLD_NS
