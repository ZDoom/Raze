//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors
Copyright (C) 2020 Nuke.YKT

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

#pragma once

BEGIN_RR_NS

int rrdh_random(void);
int ghcons_isanimalescapewall(short w);
int ghcons_isanimalescapesect(short s);
int ghtrophy_isakill(short a1);
int sub_535EC(void);
void ghdeploy_bias(short a1);
int ghcons_findnewspot(short a1);
void ghtrax_leavedroppings(short a1);
void ghtrax_leavetrax(short a1);
int ghtrax_isplrupwind(short a1, short a2);
int ghsound_pfiredgunnear(spritetype* a1, short a2);
int ghsound_pmadesound(spritetype* a1, short a2);
int ghsound_pmadecall(spritetype* a1, short a2);
void sub_5A250(int a1);
void sub_53304(void);
void sub_54DE0(void);
void ghtrophy_loadbestscores(void);
void sub_5A02C(void);
void sub_579A0(void);
void ghsound_ambientlooppoll(void);
int sub_51B68(void);
void sub_5469C(vec2_t const origin, int a1);
void ghstatbr_registerkillinfo(short a1, int a2, int a3);
char sub_54B80(void);

END_RR_NS
