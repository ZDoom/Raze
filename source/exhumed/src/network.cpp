//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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
#include "ns.h"
// this is net.c in the original code

#include "typedefs.h"
#include "network.h"
#include "ps_input.h"

BEGIN_PS_NS

short nNetMoveFrames = 0;


void SendGoodbye()
{
    UpdateInputs();
}

void UpdateNetInputs()
{

}
/*
int InitNet(short nSocket, int nPlayers)
{
    return 0;
}
*/

void AbortNetworkPlay()
{

}

void UnInitNet()
{

}
END_PS_NS
