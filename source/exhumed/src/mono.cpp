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

/*
Name:  MonoClear_
address      = 0001:0001CF5A
module index = 24
kind:          (code)
Name:  MonoInUse_
address      = 0001:0001CF8A
module index = 24
kind:          (code)
Name:  MonoOpen_
address      = 0001:0001CF8A
module index = 24
kind:          (code)
Name:  MonoClose_
address      = 0001:0001CF8D
module index = 24
kind:          (code)
Name:  MonoOut_
address      = 0001:0001CFAA
module index = 24
kind:          (code)
Name:  CACopy_
address      = 0001:0001D1C0
module index = 24
kind:          (static pubdef) (code)
Name:  CAFill_
address      = 0001:0001D1CD
module index = 24
kind:          (static pubdef) (code)
Name:  MonoQuery_
address      = 0001:0001D1E6
module index = 24
kind:          (code)
Name:  _rowCur
address      = 0003:000073D8
module index = 24
kind:          (data)
Name:  _colCur
address      = 0003:000073DC
module index = 24
kind:          (data)
Name:  _fMonoOpen
address      = 0003:000073E0
module index = 24
kind:          (data)


*/
#include "mono.h"

int rowCur = 0;
int colCur = 0;
