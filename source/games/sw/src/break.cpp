//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#include "ns.h"
#include "build.h"

#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "sprite.h"
#include "sector.h"
#include "light.h"
#include "weapon.h"

#include "break.h"

BEGIN_SW_NS

static void DoWallBreakSpriteMatch(int match);

BREAK_INFO WallBreakInfo[] =
{
    { 60,  -1, SHRAP_METAL, BF_KILL, 0 },
    { 82,  -1, SHRAP_METAL, BF_KILL, 0 },
    { 1, 3593, SHRAP_METAL, 0, 0 },
//{ 13, -1, SHRAP_PAPER },
//{ 14, -1, SHRAP_PAPER },
//{ 49, -1, SHRAP_PAPER },
//{ 52, -1, SHRAP_PAPER },
//{ 99, -1, SHRAP_PAPER },
//{ 102, -1, SHRAP_PAPER },
//{ 207, -1, SHRAP_PAPER },
    { 253, 255, SHRAP_GLASS, 0, 0 },
    { 254, 255, SHRAP_GLASS, 0, 0 },
    { 282, -1, SHRAP_GLASS, 0, 0 },
    { 283, 4974, SHRAP_METAL, 0, 0 },
    { 318, 599, SHRAP_GLASS, 0, 0},
    { 486, -1, SHRAP_METAL, 0, 0 },
    { 487, 3676, SHRAP_METAL, 0, 0 },
    { 628, 3585, SHRAP_METAL, 0, 0},
    { 630, 3586, SHRAP_METAL, 0, 0},
    { 633, 608, SHRAP_GLASS, 0, 0},
    { 634, 608, SHRAP_GLASS, 0, 0 },
    { 637, 3587, SHRAP_METAL, 0, 0},
    { 640, 3588, SHRAP_METAL, 0, 0},
    { 641, 3588, SHRAP_METAL, 0, 0},
    { 665, 3588, SHRAP_METAL, 0, 0},
    { 742, 3589, SHRAP_COIN, 0, 0},
    { 743, 3590, SHRAP_COIN, 0, 0},
    { 750, 608, SHRAP_GLASS, 0, 0},
    { 2667, 608, SHRAP_GLASS, 0, 0},
    { 2769, 3681, SHRAP_GLASS, 0, 0},
    { 2676, 3591, SHRAP_GLASS, 0, 0},
    { 2677, 3592, SHRAP_GLASS, 0, 0},
    { 2687, 2727, SHRAP_GLASS, 0, 0},
    { 2688, 2728, SHRAP_GLASS, 0, 0},
//{ 2714, 3593, SHRAP_GLASS},
    { 2732, 3594, SHRAP_GLASS, 0, 0},
    { 2777, 3683, SHRAP_METAL, 0, 0},
    { 2778, 2757, SHRAP_GLASS, 0, 0},
    { 2801, 3591, SHRAP_GLASS, 0, 0},
    { 2804, 3595, SHRAP_GLASS, 0, 0},
    { 2807, 3596, SHRAP_GLASS, 0, 0},
    { 2810, 4989, SHRAP_METAL, 0, 0},
    { 4890, 4910, SHRAP_METAL, 0, 0},
    { 4891, 4911, SHRAP_METAL, 0, 0},
    { 4892, 4912, SHRAP_METAL, 0, 0},
    { 4893, 4913, SHRAP_METAL, 0, 0},
    { 4894, 4914, SHRAP_METAL, 0, 0},
    { 4895, 4915, SHRAP_METAL, 0, 0},
    { 3336, 4940, SHRAP_COIN, 0, 0},
    { 3337, 4941, SHRAP_COIN, 0, 0},
    { 4885, 4888, SHRAP_METAL, 0, 0},
    { 4887, 4889, SHRAP_COIN, 0, 0},
    { 3350, 4942, SHRAP_GLASS, 0, 0},
    { 3351, 4943, SHRAP_METAL, 0, 0},
    { 3352, 4944, SHRAP_METAL, 0, 0},
    { 3353, 4945, SHRAP_METAL, 0, 0},
    { 4896, 4898, SHRAP_METAL, 0, 0},
    { 4897, 4899, SHRAP_METAL, 0, 0},
    { 3385, 4981, SHRAP_METALMIX, 0, 0},
    { 3389, 4982, SHRAP_METALMIX, 0, 0},
    { 3393, 4984, SHRAP_METALMIX, 0, 0},
    { 3397, 4983, SHRAP_METALMIX, 0, 0},
    { 3401, 4985, SHRAP_METALMIX, 0, 0},
    { 3405, 4986, SHRAP_METALMIX, 0, 0},
    { 3409, 4988, SHRAP_METALMIX, 0, 0},
    { 3413, 4987, SHRAP_METALMIX, 0, 0},
    { 253, 255,   SHRAP_METALMIX, 0, 0},
    { 283, 4974,  SHRAP_METALMIX, 0, 0},
    { 299, 4975,  SHRAP_METALMIX, 0, 0},
    {5078, 5079,  SHRAP_METALMIX, 0, 0},
    {5080, 5092,  SHRAP_MARBELS, 0, 0},
    {5083, 5093,  SHRAP_MARBELS, 0, 0},
    {5086, 5094,  SHRAP_MARBELS, 0, 0},
    {5089, 5095,  SHRAP_MARBELS, 0, 0},
    {4970, 4973,  SHRAP_METAL, 0, 0},
    {297,  4980,  SHRAP_METAL, 0, 0},
    {1,    4976,  SHRAP_METAL, 0, 0},
    {4917, 4918,  SHRAP_METAL, 0, 0},
    {4902, 4903,  SHRAP_METAL, 0, 0},
};

BREAK_INFO SpriteBreakInfo[] =
{
    { 60,  -1, SHRAP_METAL, BF_KILL, 0},
    { 82,  -1, SHRAP_METAL, BF_KILL, 0},
    { 138, -1, SHRAP_GENERIC, BF_KILL, 0},
    { 253, 255, SHRAP_GLASS, 0, 0},
    { 254, 255, SHRAP_GLASS, 0, 0},
    { 270, -1, SHRAP_PAPER, BF_BURN, 0},
    { 271, -1, SHRAP_PAPER, BF_BURN, 0},
    { 272, -1, SHRAP_WOOD, 0, 0},
    { 274, -1, SHRAP_PAPER, BF_BURN, 0},
//{ 276, -1, SHRAP_WOOD },
//{ 277, -1, SHRAP_WOOD },
//{ 278, -1, SHRAP_WOOD },
    { 282, -1, SHRAP_GLASS, 0, 0},
    { 283, -1, SHRAP_METAL, 0, 0},
    { 297, -1, SHRAP_METAL, 0, 0},
    { 299, -1, SHRAP_METAL, 0, 0},
    { 363, -1, SHRAP_METAL, BF_KILL, 0},
    { 365, -1, SHRAP_STONE, BF_TOUGH|BF_KILL, 0},
    { 366, -1, SHRAP_METAL, BF_KILL,5},
    { 367, -1, SHRAP_WOOD, BF_KILL, 0},
    { 368, -1, SHRAP_GIBS, BF_KILL, 0},
    { 369, -1, SHRAP_WOOD, BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { 371, -1, SHRAP_WOOD, BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { 372, -1, SHRAP_WOOD, BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { 374, -1, SHRAP_STONE, BF_TOUGH|BF_KILL, 0},
    { 375, -1, SHRAP_STONE, BF_TOUGH|BF_KILL, 0},
    { 376, -1, SHRAP_WOOD, BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { 377, -1, SHRAP_STONE, BF_KILL, 0},
    { 379, -1, SHRAP_WOOD, BF_KILL, 0},
    { 380, -1, SHRAP_METAL, BF_KILL|BF_FIRE_FALL, 0},
    { 385, -1, SHRAP_BLOOD, BF_KILL, 0},
    { 386, -1, SHRAP_GIBS, BF_KILL, 0},
    { 387, -1, SHRAP_GIBS, BF_KILL, 0},
    { 388, -1, SHRAP_GIBS, BF_KILL|BF_TOUGH, 0},
    { 391, -1, SHRAP_GIBS, BF_KILL, 0},
    { 392, -1, SHRAP_GIBS, BF_KILL, 0},
    { 393, -1, SHRAP_WOOD, BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { 394, -1, SHRAP_GIBS, BF_KILL|BF_TOUGH, 0},
    { 395, -1, SHRAP_GIBS, BF_KILL, 0},
    { 396, -1, SHRAP_METAL, BF_KILL|BF_FIRE_FALL, 0},
    { 397, -1, SHRAP_METAL, BF_KILL|BF_FIRE_FALL, 0},
    { 398, -1, SHRAP_METAL, BF_KILL|BF_FIRE_FALL, 0},
    { 399, -1, SHRAP_METAL, BF_KILL|BF_FIRE_FALL, 0},
    { 400, -1, SHRAP_GENERIC, BF_KILL, 0},
    { 401, -1, SHRAP_WOOD, BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { 402, -1, SHRAP_WOOD, BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { 407, -1, SHRAP_METAL, BF_KILL, 0},
    { 408, -1, SHRAP_PAPER, BF_BURN, 0},
    { 409, -1, SHRAP_PAPER, BF_BURN, 0},
    { 415, -1, SHRAP_METAL, BF_KILL|BF_FIRE_FALL,5},
    { 418, -1, SHRAP_GENERIC, BF_KILL|BF_FIRE_FALL,5},
    { 422, -1, SHRAP_METAL, BF_KILL, 0},
    { 423, -1, SHRAP_BLOOD, BF_KILL, 0},
    { 424, -1, SHRAP_BLOOD, BF_KILL, 0},
    { 425, -1, SHRAP_BLOOD, BF_KILL, 0},
    { 428, -1, SHRAP_METAL, BF_BURN, 0},
    { 430, -1, SHRAP_STONE, BF_KILL|BF_FIRE_FALL, 0},
    { 431, -1, SHRAP_STONE, BF_KILL|BF_FIRE_FALL, 0},
    { 432, -1, SHRAP_STONE, BF_KILL|BF_FIRE_FALL, 0},
    { 433, -1, SHRAP_STONE, BF_KILL|BF_FIRE_FALL, 0},
    { 434, -1, SHRAP_STONE, BF_KILL|BF_FIRE_FALL, 0},
    { 435, -1, SHRAP_STONE, BF_KILL|BF_FIRE_FALL, 0},
    { 436, -1, SHRAP_STONE, BF_KILL|BF_FIRE_FALL, 0},
    { 437, -1, SHRAP_STONE, BF_KILL|BF_FIRE_FALL, 0},
    { 438, -1, SHRAP_GIBS, BF_KILL, 0},
    { 441, -1, SHRAP_WOOD, BF_KILL, 0},
    { 442, -1, SHRAP_STONE, BF_KILL, 0},
    { 443, -1, SHRAP_STONE, BF_KILL|BF_FIRE_FALL, 0},
    { 453, -1, SHRAP_WOOD, BF_KILL, 0},
    { 458, -1, SHRAP_STONE, BF_KILL, 0},
    { 459, -1, SHRAP_STONE, BF_KILL, 0},
    { 460, -1, SHRAP_METAL, BF_KILL, 0},
    { 461, -1, SHRAP_METAL, BF_KILL, 0},
    { 462, -1, SHRAP_METAL, BF_KILL, 0},
    { 463, -1, SHRAP_STONE, BF_KILL, 0},
    { 467, -1, SHRAP_STONE, BF_KILL, 0},
    { 468, -1, SHRAP_WOOD, BF_KILL, 0},
    { 475, -1, SHRAP_GLASS, BF_KILL, 0},
    { 481, -1, SHRAP_GENERIC, BF_KILL, 0},
    { 482, -1, SHRAP_WOOD, BF_KILL, 0},
    { 483, -1, SHRAP_WOOD, BF_KILL|BF_TOUGH, 0},
    { 491, -1, SHRAP_WOOD, BF_KILL, 0},
    { 492, -1, SHRAP_METAL, BF_KILL, 0},
    { 493, -1, SHRAP_METAL, BF_KILL, 0},
    { 498, -1, SHRAP_GENERIC, BF_KILL, 0},
    { 500, -1, SHRAP_METAL, BF_KILL, 0},
    { 501, -1, SHRAP_METAL, BF_KILL, 0},
    { 504, -1, SHRAP_METAL, BF_KILL,5},
    { 505, -1, SHRAP_BLOOD, BF_KILL,5},
    { 506, -1, SHRAP_GENERIC, BF_KILL,5},
    { 507, -1, SHRAP_GLASS, BF_KILL, 0},
    { 508, -1, SHRAP_GLASS, BF_KILL, 0},
    { 509, -1, SHRAP_GLASS, BF_KILL, 0},
    { 510, -1, SHRAP_GLASS, BF_KILL, 0},
    { 511, -1, SHRAP_METAL, BF_KILL, 0},
    { 512, -1, SHRAP_METAL, BF_KILL|BF_FIRE_FALL,5},
    { 516, -1, SHRAP_WOOD, BF_BURN, 0},
    { 517, -1, SHRAP_WOOD, BF_BURN, 0},
    { 518, -1, SHRAP_WOOD, BF_BURN, 0},
    { 519, -1, SHRAP_WOOD, BF_FIRE_FALL|BF_KILL,5},
    { 520, -1, SHRAP_WOOD, BF_KILL, 0},
    { 521, -1, SHRAP_WOOD, BF_KILL|BF_FIRE_FALL, 0},
    { 537, -1, SHRAP_METAL, BF_KILL|BF_FIRE_FALL, 0},
    { 541, -1, SHRAP_WOOD, BF_KILL|BF_FIRE_FALL, 0},
    { 586, -1, SHRAP_METAL, BF_KILL, 0},
    { 590, -1, SHRAP_METAL, BF_KILL, 0},
    { 591, -1, SHRAP_METAL, BF_KILL, 0},
    { 593, 608, SHRAP_GLASS,BF_TOUGH, 0},
    { 604, -1, SHRAP_METAL, BF_KILL, 0},
    { 613, -1, SHRAP_LARGE_EXPLOSION, BF_KILL, 0},
    { 614, -1, SHRAP_METAL, BF_KILL, 0},
    { 615, -1, SHRAP_METAL, BF_KILL, 0},
    { 618, -1, SHRAP_GLASS, BF_KILL, 0},
    { 646, -1, SHRAP_METAL, BF_KILL, 0},
    { 647, -1, SHRAP_LARGE_EXPLOSION, BF_KILL, 0},
    { 648, -1, SHRAP_LARGE_EXPLOSION, BF_KILL, 0},
    { 649, -1, SHRAP_METAL, BF_KILL, 0},
    { 656, -1, SHRAP_METAL, BF_KILL, 0},
    { 657, -1, SHRAP_METAL, BF_KILL, 0},
    { 658, -1, SHRAP_LARGE_EXPLOSION, BF_KILL, 0},
    { 659, -1, SHRAP_METAL, BF_KILL,5},
//{ 660, -1, SHRAP_STONE, BF_TOUGH|BF_KILL},
//{ 661, -1, SHRAP_STONE, BF_TOUGH|BF_KILL},
//{ 662, -1, SHRAP_STONE, BF_TOUGH|BF_KILL},
    { 663, -1, SHRAP_METAL, BF_KILL,10},
    { 664, -1, SHRAP_METAL, BF_KILL,5},
    { 666, -1, SHRAP_PLANT, BF_KILL, 0},
    { 670, -1, SHRAP_METAL, BF_KILL|BF_FIRE_FALL, 0},
    { 671, -1, SHRAP_GLASS, BF_KILL|BF_FIRE_FALL, 0},
    { 673, -1, SHRAP_BLOOD, BF_KILL, 0},
    { 674, -1, SHRAP_GIBS, BF_KILL, 0},
    { 675, -1, SHRAP_GIBS, BF_KILL, 0},
    { 676, -1, SHRAP_GIBS, BF_KILL, 0},
    { 678, -1, SHRAP_GLASS, BF_KILL,5},
    { 679, -1, SHRAP_GLASS, BF_KILL,5},
    { 683, -1, SHRAP_GLASS, BF_KILL,5},
    { 684, -1, SHRAP_GLASS, BF_KILL,5},
    { 685, -1, SHRAP_GLASS, BF_KILL,5},
    { 686, -1, SHRAP_PAPER, BF_KILL,5},
    { 687, -1, SHRAP_STONE, BF_KILL|BF_TOUGH, 0},
    { 688, -1, SHRAP_STONE, BF_KILL|BF_TOUGH, 0},
    { 690, -1, SHRAP_WOOD, BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { 692, -1, SHRAP_WOOD, BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { 695, -1, SHRAP_STONE, BF_KILL, 0},
    { 696, -1, SHRAP_STONE, BF_KILL, 0},
    { 697, -1, SHRAP_STONE, BF_KILL, 0},
    { 698, -1, SHRAP_STONE, BF_KILL, 0},
    { 699, -1, SHRAP_STONE, BF_TOUGH|BF_KILL, 0},
    { 702, -1, SHRAP_STONE, BF_TOUGH|BF_KILL, 0},
    { 703, -1, SHRAP_STONE, BF_TOUGH|BF_KILL, 0},
    { 704, -1, SHRAP_STONE, BF_TOUGH|BF_KILL, 0},
    { 706, -1, SHRAP_PLANT, BF_KILL, 0},
    { 707, -1, SHRAP_PLANT, BF_KILL, 0},
    { 710, -1, SHRAP_PLANT, BF_KILL, 0},
    { 711, -1, SHRAP_PLANT, BF_KILL, 0},
    { 714, -1, SHRAP_STONE, BF_KILL,5},
    { 721, -1, SHRAP_GIBS, BF_KILL, 0},
    { 722, -1, SHRAP_GIBS, BF_KILL, 0},
    { 723, -1, SHRAP_GIBS, BF_KILL, 0},
    { 724, -1, SHRAP_GIBS, BF_KILL, 0},
    { 725, -1, SHRAP_PLANT, BF_KILL, 0},
    { 730, -1, SHRAP_GENERIC, BF_KILL, 0},
    { 744, -1, SHRAP_GLASS, BF_KILL,5},
    { 2563, -1, SHRAP_PAPER, BF_BURN, 0},
    { 2564, -1, SHRAP_PAPER, BF_BURN, 0},
    { 3570, -1, SHRAP_WOOD, BF_TOUGH|BF_BURN, 0},
    { 3571, -1, SHRAP_WOOD, BF_TOUGH|BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { 3572, -1, SHRAP_WOOD, BF_TOUGH|BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { 3573, -1, SHRAP_WOOD, BF_TOUGH|BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { 3574, -1, SHRAP_WOOD, BF_TOUGH|BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { 3575, -1, SHRAP_WOOD, BF_TOUGH|BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { 3576, -1, SHRAP_WOOD, BF_TOUGH|BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { 3577, -1, SHRAP_WOOD, BF_TOUGH|BF_KILL, 0},
    { 3578, -1, SHRAP_WOOD, BF_TOUGH|BF_KILL, 0},
    { 3579, -1, SHRAP_WOOD, BF_TOUGH|BF_KILL, 0},
    { 3580, -1, SHRAP_WOOD, BF_TOUGH|BF_KILL, 0},
    { 3581, -1, SHRAP_WOOD, BF_TOUGH|BF_BURN, 0},
    { 3582, -1, SHRAP_WOOD, BF_TOUGH|BF_BURN, 0},
    { 2640, -1, SHRAP_STONE, BF_KILL,5},
    { 2641, -1, SHRAP_STONE, BF_KILL, 0},
    { 2642, -1, SHRAP_STONE, BF_KILL, 0},
    { 2680, -1, SHRAP_GENERIC, BF_KILL, 0},
    { 2681, -1, SHRAP_GENERIC, BF_KILL, 0},
    { 2682, -1, SHRAP_GENERIC, BF_KILL, 0},
    { 2683, -1, SHRAP_GENERIC, BF_KILL, 0},
    { 2684, -1, SHRAP_GENERIC, BF_KILL, 0},
    { 2685, -1, SHRAP_GENERIC, BF_KILL, 0},
    { 2687, 2727, SHRAP_GLASS, 0, 0},
    { 2688, 2728, SHRAP_GLASS, 0, 0},
    { 2699, -1, SHRAP_WOOD, BF_KILL, 0},
    { 2709, -1, SHRAP_WOOD, BF_TOUGH|BF_KILL, 0},
    { 2720, -1, SHRAP_GIBS, BF_KILL, 0},
    { 2721, -1, SHRAP_GIBS, BF_KILL, 0},
    { 2722, -1, SHRAP_GIBS, BF_KILL, 0},
    { 2723, -1, SHRAP_GIBS, BF_KILL, 0},
    { 2724, -1, SHRAP_GIBS, BF_KILL, 0},
    { 2725, -1, SHRAP_BLOOD, BF_KILL, 0},
    { 2726, -1, SHRAP_BLOOD, BF_KILL, 0},
    { 2719, -1, SHRAP_GLASS, BF_KILL, 0},
    { 2750, -1, SHRAP_WOOD, BF_KILL, 0},
    { 2676, 3591, SHRAP_GLASS, 0, 0},
    { 2769, 3681, SHRAP_GLASS, 0, 0},
    { 2777, 3683, SHRAP_METAL, BF_TOUGH, 0},
    { 2778, 2757, SHRAP_GLASS, 0, 0},
    { 3448, 3451, SHRAP_METAL, BF_TOUGH|BF_KILL, 0},
    { 3449, -1, SHRAP_PAPER, BF_KILL, 0},
    { 3497, -1, SHRAP_GENERIC, BF_KILL|BF_TOUGH, 0},
    { 3551, -1, SHRAP_METAL, BF_KILL, 0},
    { 3552, -1, SHRAP_METAL, BF_KILL, 0},
    { 3553, -1, SHRAP_METAL, BF_KILL, 0},
    { 3554, -1, SHRAP_METAL, BF_KILL, 0},
    { 3555, -1, SHRAP_METAL, BF_KILL, 0},
    { 3556, -1, SHRAP_METAL, BF_KILL, 0},
    { 3557, -1, SHRAP_METAL, BF_KILL, 0},
    { 3558, -1, SHRAP_WOOD, BF_KILL, 0},
    { 3568, -1, SHRAP_WOOD, BF_BURN, 0},
    { 4994, -1, SHRAP_METAL, BF_KILL, 0},
    { 4995, -1, SHRAP_METAL, BF_KILL, 0},
    { 5010, -1, SHRAP_WOOD, BF_TOUGH|BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { 5017, -1, SHRAP_PAPER, BF_KILL, 0},
    { 5018, -1, SHRAP_PAPER, BF_KILL, 0},
    { 5019, -1, SHRAP_PAPER, BF_KILL, 0},
    { 5060, -1, SHRAP_METAL, BF_KILL, 0},
    { 5061, -1, SHRAP_METAL, BF_KILL, 0},
    { 5073, -1, SHRAP_GIBS, BF_KILL, 0},

// Evil ninja Hari-Kari - can gib
    { 4218, -1, SHRAP_GIBS, BF_KILL|BF_TOUGH, 0},

// old ninja dead frames
    { 1133, -1, SHRAP_GIBS, BF_KILL|BF_TOUGH, 0},
    { 1134, -1, SHRAP_GIBS, BF_KILL|BF_TOUGH, 0},

// dead actors
    { 811, -1, SHRAP_GIBS, BF_KILL|BF_TOUGH, 0},
    { 1440, -1, SHRAP_GIBS, BF_KILL|BF_TOUGH, 0},
    { 1512, -1, SHRAP_GIBS, BF_KILL|BF_TOUGH, 0},
    { 1643, -1, SHRAP_GIBS, BF_KILL|BF_TOUGH, 0},
    { 1680, -1, SHRAP_GIBS, BF_KILL|BF_TOUGH, 0},

    { 4219+7, -1, SHRAP_GIBS, BF_KILL|BF_TOUGH, 0},
    { 4236, -1, SHRAP_GIBS, BF_KILL|BF_TOUGH, 0},  // Evil Ninja cut in half
    { 4421, -1, SHRAP_GIBS, BF_KILL|BF_TOUGH, 0},  // Dead Big Ripper
    { 4312, -1, SHRAP_GIBS, BF_KILL|BF_TOUGH, 0},  // Dead Coolie Ghost
    { 766, -1, SHRAP_COIN,BF_KILL, 0},
    { 767, -1, SHRAP_COIN,BF_KILL, 0},
    { 2700, -1, SHRAP_PAPER,BF_KILL, 0},
    { 2701, -1, SHRAP_PAPER,BF_KILL, 0},
    { 2702, -1, SHRAP_PAPER,BF_KILL, 0},
    { 2703, -1, SHRAP_PAPER,BF_KILL, 0},
    { 2704, -1, SHRAP_PAPER,BF_KILL, 0},
    { 2705, -1, SHRAP_PAPER,BF_KILL, 0},
    { 2218, -1, SHRAP_METAL,BF_KILL, 0},   // Caltrops are breakable
    { 689,  -1, SHRAP_METAL,BF_TOUGH|BF_KILL, 0},
//{ 2540, -1, SHRAP_METAL,BF_TOUGH|BF_KILL},
    { 3354, -1, SHRAP_METAL,BF_KILL, 0},
    { 3357, -1, SHRAP_PAPER,BF_KILL, 0},
    { 4886, -1, SHRAP_GLASS,BF_KILL, 0},
    { 646, 708, SHRAP_METAL,BF_TOUGH|BF_KILL, 0},
    { 708, -1,  SHRAP_METAL,BF_TOUGH|BF_KILL, 0},
    { 656, -1,  SHRAP_METAL,BF_KILL, 0},
    { 663, -1,  SHRAP_METAL,BF_KILL, 0},
    { 664, -1,  SHRAP_METAL,BF_KILL, 0},
    { 691, -1,  SHRAP_METAL,BF_KILL, 0},
    { 5021,-1,  SHRAP_GIBS, BF_KILL, 0},
    { 712, -1,  SHRAP_LARGE_EXPLOSION, BF_KILL, 0},
    { 713, -1,  SHRAP_LARGE_EXPLOSION, BF_KILL, 0},
    { 693, -1,  SHRAP_WOODMIX, BF_KILL|BF_TOUGH, 0},
    { 5041,-1,  SHRAP_GIBS, BF_KILL, 0},
    { 5042,5077,SHRAP_GIBS, BF_TOUGH|BF_KILL, 0},
    { 5077,-1,  SHRAP_WOOD, BF_TOUGH|BF_KILL, 0},
    { 3356,3358,SHRAP_WOOD, BF_TOUGH|BF_KILL, 0},
    { 3358,-1,  SHRAP_WOOD, BF_TOUGH|BF_KILL, 0},
    { 900, -1,  SHRAP_GIBS, BF_KILL, 0},
    { 901, -1,  SHRAP_GIBS, BF_KILL, 0},
    { 902, -1,  SHRAP_GIBS, BF_KILL, 0},
    { 915, -1,  SHRAP_GIBS, BF_KILL, 0},
    { 916, -1,  SHRAP_GIBS, BF_KILL, 0},
    { 917, -1,  SHRAP_GIBS, BF_KILL, 0},
    { 930, -1,  SHRAP_GIBS, BF_KILL, 0},
    { 931, -1,  SHRAP_GIBS, BF_KILL, 0},
    { 932, -1,  SHRAP_GIBS, BF_KILL, 0},
    { 1670,-1,  SHRAP_GIBS, BF_KILL, 0},
    { 2219,-1,  SHRAP_METAL,BF_KILL|BF_TOUGH, 0},
    { 4768,-1,  SHRAP_GLASS,BF_KILL, 0},
    { 4792,-1,  SHRAP_GLASS,BF_KILL, 0},
    { 4816,-1,  SHRAP_GLASS,BF_KILL, 0},
    { 4840,-1,  SHRAP_GLASS,BF_KILL, 0},
    { 4584,-1,  SHRAP_GIBS, BF_KILL, 0},
    { 5062,-1,  SHRAP_WOOD, BF_KILL|BF_TOUGH, 0},
    { 5063,4947,SHRAP_PAPERMIX, BF_KILL|BF_TOUGH|BF_LEAVE_BREAK, 0},
    { 4947,-1,  SHRAP_PAPERMIX, BF_KILL|BF_TOUGH, 0},
    { 1160,-1,  SHRAP_GIBS, BF_KILL|BF_TOUGH, 0},
    { 5104,-1,  SHRAP_GIBS, BF_KILL, 0},
    { 3795,-1,  SHRAP_GIBS, BF_KILL, 0},
    { 470,-1,   SHRAP_GIBS, BF_KILL, 0},
    { 5205,-1,  SHRAP_GIBS, BF_KILL|BF_TOUGH, 0},
//{ 969,-1,   SHRAP_GIBS, BF_KILL|BF_TOUGH},
//{ 1277,-1,  SHRAP_GIBS, BF_KILL|BF_TOUGH},
};

//////////////////////////////////////////////
// SORT & SEARCH SUPPORT
//////////////////////////////////////////////

static int CompareBreakInfo(void const * a, void const * b)
{
    auto break_info1 = (BREAK_INFO const *)a;
    auto break_info2 = (BREAK_INFO const *)b;

    // will return a number less than 0 if break_info1 < break_info2
    return break_info1->picnum - break_info2->picnum;
}

int CompareSearchBreakInfo(int *picnum, BREAK_INFO* break_info)
    {
    // will return a number less than 0 if picnum < break_info->picnum
    return(*picnum - break_info->picnum);
    }

BREAK_INFO* FindWallBreakInfo(int picnum)
    {
    return(BREAK_INFO*)(bsearch(&picnum, &WallBreakInfo, SIZ(WallBreakInfo), sizeof(BREAK_INFO), (int(*)(const void*,const void*))CompareSearchBreakInfo));
    }

BREAK_INFO* FindSpriteBreakInfo(int picnum)
    {
    return(BREAK_INFO*)(bsearch(&picnum, &SpriteBreakInfo, SIZ(SpriteBreakInfo), sizeof(BREAK_INFO), (int(*)(const void*,const void*))CompareSearchBreakInfo));
    }

//////////////////////////////////////////////
// SETUP
//////////////////////////////////////////////

void SortBreakInfo(void)
{
    qsort(&SpriteBreakInfo, SIZ(SpriteBreakInfo), sizeof(BREAK_INFO), CompareBreakInfo);
    qsort(&WallBreakInfo, SIZ(WallBreakInfo), sizeof(BREAK_INFO), CompareBreakInfo);
}

BREAK_INFO* SetupWallForBreak(walltype* wallp)
{
    BREAK_INFO* break_info;

    break_info = FindWallBreakInfo(wallp->picnum);
    if (break_info)
    {
        wallp->lotag = TAG_WALL_BREAK;
        wallp->extra |= (WALLFX_DONT_STICK);
    }

    if (wallp->overpicnum > 0 && (wallp->cstat & CSTAT_WALL_MASKED))
    {
        break_info = FindWallBreakInfo(wallp->overpicnum);
        if (break_info)
        {
            wallp->lotag = TAG_WALL_BREAK;
            wallp->extra |= (WALLFX_DONT_STICK);
        }
    }

    return break_info;
}

BREAK_INFO* SetupSpriteForBreak(DSWActor* actor)
{
    int picnum = actor->spr.picnum;
    BREAK_INFO* break_info;

    // ignore as a breakable if true
    if (actor->spr.lotag == TAG_SPRITE_HIT_MATCH)
        return nullptr;

    break_info = FindSpriteBreakInfo(picnum);
    if (break_info)
    {

        // use certain sprites own blocking for determination
        if ((break_info->flags & BF_OVERRIDE_BLOCK))
        {
            // if not blocking then skip this code
            if (!(actor->spr.cstat & CSTAT_SPRITE_BLOCK))
            {
                return (BREAK_INFO*)(-1);
            }
        }

        if ((break_info->flags & BF_BURN))
            actor->spr.extra |= (SPRX_BURNABLE);
        else
            actor->spr.extra |= (SPRX_BREAKABLE);

        actor->spr.clipdist = ActorSizeX(actor);

        actor->spr.cstat |= (CSTAT_SPRITE_BREAKABLE);
    }

    return break_info;
}

//////////////////////////////////////////////
// ACTIVATE
//////////////////////////////////////////////

DSWActor* FindBreakSpriteMatch(int match)
{
    SWStatIterator it(STAT_BREAKABLE);
    while (auto actor = it.Next())
    {
        if (SP_TAG2(actor) == match && actor->spr.picnum == ST1)
        {
            return actor;
        }
    }

    return nullptr;
}

//
// WALL
//

int AutoBreakWall(walltype* wallp, int hit_x, int hit_y, int hit_z, int ang, int type)
{
    BREAK_INFO* break_info;
    walltype* nwp;

    wallp->lotag = 0;
    if (wallp->twoSided())
    {
        nwp = wallp->nextWall();

        // get rid of both sides
        // only break ONE of the walls

        if (nwp->lotag == TAG_WALL_BREAK &&
            nwp->overpicnum > 0 &&
            (nwp->cstat & CSTAT_WALL_MASKED))
        {
            nwp->lotag = 0;
        }
    }

    if (wallp->overpicnum > 0 && (wallp->cstat & CSTAT_WALL_MASKED))
        break_info = FindWallBreakInfo(wallp->overpicnum);
    else
        break_info = FindWallBreakInfo(wallp->picnum);

    if (!break_info)
    {
        return false;
    }

    // Check to see if it should break with current weapon type
    if (!CheckBreakToughness(break_info, type)) return false;

    if (hit_x != INT32_MAX)
    {
        vec3_t hit_pos = { hit_x, hit_y, hit_z };
        // need correct location for spawning shrap
        auto breakActor = insertActor(0, STAT_DEFAULT);
        breakActor->spr.cstat = 0;
        breakActor->spr.extra = 0;
        breakActor->spr.ang = ang;
        breakActor->spr.picnum = ST1;
        breakActor->spr.xrepeat = breakActor->spr.yrepeat = 64;
        SetActorZ(breakActor, &hit_pos);
        SpawnShrap(breakActor, nullptr, -1, break_info);
        KillActor(breakActor);
    }

    // change the wall
    if (wallp->overpicnum > 0 && (wallp->cstat & CSTAT_WALL_MASKED))
    {
        if (break_info->breaknum == -1)
        {
            wallp->cstat &= ~(CSTAT_WALL_MASKED|CSTAT_WALL_1WAY|CSTAT_WALL_BLOCK_HITSCAN|CSTAT_WALL_BLOCK);
            wallp->overpicnum = 0;
            if (wallp->twoSided())
            {
                nwp = wallp->nextWall();
                nwp->cstat &= ~(CSTAT_WALL_MASKED|CSTAT_WALL_1WAY|CSTAT_WALL_BLOCK_HITSCAN|CSTAT_WALL_BLOCK);
                nwp->overpicnum = 0;
            }
        }
        else
        {
            wallp->cstat &= ~(CSTAT_WALL_BLOCK_HITSCAN|CSTAT_WALL_BLOCK);
            wallp->overpicnum = break_info->breaknum;
            if (wallp->twoSided())
            {
                nwp = wallp->nextWall();
                nwp->cstat &= ~(CSTAT_WALL_BLOCK_HITSCAN|CSTAT_WALL_BLOCK);
                nwp->overpicnum = break_info->breaknum;
            }
        }
    }
    else
    {
        if (break_info->breaknum == -1)
            wallp->picnum = 594; // temporary break pic
        else
        {
            wallp->picnum = break_info->breaknum;
            if ((int16_t)wallp->hitag < 0)
                DoWallBreakSpriteMatch(wallp->hitag);
        }
    }
    return true;
}

bool UserBreakWall(walltype* wp)
{
    int match = wp->hitag;
    const auto block_flags = CSTAT_WALL_BLOCK|CSTAT_WALL_BLOCK_HITSCAN;
    const auto type_flags = CSTAT_WALL_TRANSLUCENT|CSTAT_WALL_MASKED|CSTAT_WALL_1WAY;
    const auto flags = block_flags|type_flags;
    bool ret = false;

    auto actor = FindBreakSpriteMatch(match);

    if (actor == nullptr)
    {
        // do it the old way and get rid of wall - assumed to be masked
        DoSpawnSpotsForKill(match);
        wp->cstat &= ~(flags);
        if (wp->twoSided())
            wp->nextWall()->cstat &= ~(flags);

        // clear tags
        wp->hitag = wp->lotag = 0;
        if (wp->twoSided())
            wp->nextWall()->hitag = wp->nextWall()->lotag = 0;
        return true;
    }

    if (wp->picnum == SP_TAG5(actor))
        return true;

    // make it BROKEN
    if (SP_TAG7(actor) <= 1)
    {
        DoSpawnSpotsForKill(match);
        DoLightingMatch(match, -1);

        if (SP_TAG8(actor) == 0)
        {
            wp->picnum = SP_TAG5(actor);
            // clear tags
            wp->hitag = wp->lotag = 0;
            if (wp->twoSided())
                wp->nextWall()->hitag = wp->nextWall()->lotag = 0;
            ret = false;
        }
        else if (SP_TAG8(actor) == 1)
        {
            // clear flags
            wp->cstat &= ~(flags);
            if (wp->twoSided())
                wp->nextWall()->cstat &= ~(flags);
            // clear tags
            wp->hitag = wp->lotag = 0;
            if (wp->twoSided())
                wp->nextWall()->hitag = wp->nextWall()->lotag = 0;

            ret = true;
        }
        else if (SP_TAG8(actor) == 2)
        {
            // set to broken pic
            wp->picnum = SP_TAG5(actor);

            // clear flags
            wp->cstat &= ~(block_flags);
            if (wp->twoSided())
                wp->nextWall()->cstat &= ~(block_flags);

            // clear tags
            wp->hitag = wp->lotag = 0;
            if (wp->twoSided())
                wp->nextWall()->hitag = wp->nextWall()->lotag = 0;

            ret = false;
        }

        return ret;
    }
    else
    {
        // increment picnum
        wp->picnum++;

        DoSpawnSpotsForDamage(match);
    }

    return false;
}

int WallBreakPosition(walltype* wp, sectortype** sectp, int *x, int *y, int *z, int *ang)
{
    int nx,ny;
    int wall_ang;

    wall_ang = NORM_ANGLE(getangle(wp->delta())+512);

    *sectp = wp->sectorp();
    ASSERT(*sectp);

    // midpoint of wall
    *x = (wp->pos.X + wp->pos.X) >> 1;
    *y = (wp->pos.Y + wp->pos.Y) >> 1;

    if (!wp->twoSided())
    {
        // white wall
        *z = ((*sectp)->floorz + (*sectp)->ceilingz) >> 1;
    }
    else
    {
        auto next_sect = wp->nextSector();

        // red wall
        ASSERT(wp->twoSided());

        // floor and ceiling meet
        if (next_sect->floorz == next_sect->ceilingz)
            *z = ((*sectp)->floorz + (*sectp)->ceilingz) >> 1;
        else
        // floor is above other sector
        if (next_sect->floorz < (*sectp)->floorz)
            *z = (next_sect->floorz + (*sectp)->floorz) >> 1;
        else
        // ceiling is below other sector
        if (next_sect->ceilingz > (*sectp)->ceilingz)
            *z = (next_sect->ceilingz + (*sectp)->ceilingz) >> 1;
    }

    *ang = wall_ang;

    nx = MOVEx(128, wall_ang);
    ny = MOVEy(128, wall_ang);

    *x += nx;
    *y += ny;

    updatesectorz(*x,*y,*z,sectp);
    if (*sectp == nullptr)
    {
        *x = INT32_MAX;  // don't spawn shrap, just change wall
        return false;
    }

    return true;
}

// If the tough parameter is not set, then it can't break tough walls and sprites
bool HitBreakWall(walltype* wp, int hit_x, int hit_y, int hit_z, int ang, int type)
{
    int match = wp->hitag;

    if (match > 0)
    {
        UserBreakWall(wp);
        return true;
    }

    //if (hit_x == INT32_MAX)
    {
        sectortype* sect = nullptr;
        WallBreakPosition(wp, &sect, &hit_x, &hit_y, &hit_z, &ang);
    }

    AutoBreakWall(wp, hit_x, hit_y, hit_z, ang, type);
    return true;
}

//
// SPRITE
//

int KillBreakSprite(DSWActor* breakActor)
{
    // Double deletion can easily happen with the break sprite code.
    if (breakActor->ObjectFlags & OF_EuthanizeMe)
        return false;


    // Does not actually kill the sprite so it will be valid for the rest
    // of the loop traversal.

    // IMPORTANT: Do not change the statnum if possible so that NEXTI in
    // SpriteControl loop traversals will maintain integrity.

    SpriteQueueDelete(breakActor);

    if (breakActor->hasU())
    {
        if (breakActor->spr.statnum == STAT_DEFAULT)
            // special case allow kill of sprites on STAT_DEFAULT list
            // a few things have users and are not StateControlled
            KillActor(breakActor);
        else
            SetSuicide(breakActor);
    }
    else
    {
        change_actor_stat(breakActor, STAT_SUICIDE);
    }

    return 0;
}


int UserBreakSprite(DSWActor* breakActor)
{
    int match = breakActor->spr.lotag;
    int match_extra;

    auto actor = FindBreakSpriteMatch(match);

    if (actor == nullptr)
    {
        // even if you didn't find a matching ST1 go ahead and kill it and match everything
        // its better than forcing everyone to have a ST1
        DoMatchEverything(nullptr, match, -1);
        // Kill sound if one is attached
        DeleteNoSoundOwner(breakActor);
        KillBreakSprite(breakActor);
        return true;
    }

    match_extra = SP_TAG6(breakActor);

    if (breakActor->spr.picnum == SP_TAG5(actor))
        return true;

    // make it BROKEN
    if (SP_TAG7(actor) <= 1)
    {
        DoMatchEverything(nullptr, match_extra, -1);
        //DoSpawnSpotsForKill(match_extra);
        DoLightingMatch(match_extra, 0);

        if (SP_TAG8(actor) == 0)
        {
            breakActor->spr.picnum = SP_TAG5(actor);
            breakActor->spr.extra &= ~(SPRX_BREAKABLE);
        }
        else
        // kill sprite
        if (SP_TAG8(actor) == 1)
        {
            // Kill sound if one is attached
            DeleteNoSoundOwner(breakActor);
            KillBreakSprite(breakActor);
            return true;
        }
        else if (SP_TAG8(actor) == 2)
        // leave it
        {
            // set to broken pic
            breakActor->spr.picnum = SP_TAG5(actor);

            // reset
            if (SP_TAG8(actor) == 2)
            {
                breakActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
            }

            breakActor->spr.extra &= ~(SPRX_BREAKABLE);
        }
    }
    else
    {
        // increment picnum
        breakActor->spr.picnum++;

        DoSpawnSpotsForDamage(match_extra);
    }

    return false;
}

int AutoBreakSprite(DSWActor* breakActor, int type)
{
    BREAK_INFO* break_info;

    break_info = FindSpriteBreakInfo(breakActor->spr.picnum);


    if ((int16_t)breakActor->spr.hitag < 0)
        DoWallBreakMatch(breakActor->spr.hitag);

    if (!break_info)
    {
        return false;
    }

    // Check to see if it should break with current weapon type
    if (!CheckBreakToughness(break_info, type))
    {
        if (break_info->breaknum != -1)
        {
            if (!(break_info->flags & BF_LEAVE_BREAK))
            {
                breakActor->spr.extra &= ~(SPRX_BREAKABLE);
                breakActor->spr.cstat &= ~(CSTAT_SPRITE_BREAKABLE);
            }

            breakActor->spr.picnum = break_info->breaknum;
            // pass Break Info Globally
            SpawnShrap(breakActor, nullptr, -1, break_info);
            if (breakActor->spr.picnum == 3683)
                breakActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        }

        return false;
    }

    breakActor->spr.extra &= ~(SPRX_BREAKABLE);
    breakActor->spr.cstat &= ~(CSTAT_SPRITE_BREAKABLE);

    // pass Break Info Globally
    SpawnShrap(breakActor, nullptr, -1, break_info);

    // kill it or change the pic
    if ((break_info->flags & BF_KILL) || break_info->breaknum == -1)
    {
        if ((break_info->flags & BF_FIRE_FALL))
            SpawnBreakFlames(breakActor);

        breakActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        breakActor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
        // Kill sound if one is attached
        DeleteNoSoundOwner(breakActor);
        KillBreakSprite(breakActor);
        return true;
    }
    else
    {
        breakActor->spr.picnum = break_info->breaknum;
        if (breakActor->spr.picnum == 3683)
            breakActor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    }

    return false;
}

bool NullActor(DSWActor* actor)
{
    // a Null Actor is defined as an actor that has no real controlling programming attached

    // check to see if attached to SO
    if (actor->user.Flags & (SPR_SO_ATTACHED))
        return true;

    // does not have a STATE or FUNC to control it
    if (!actor->user.State)
        return true;

    // does not have a STATE or FUNC to control it
    if (!actor->user.ActorActionFunc)
        return true;

    return false;
}

int HitBreakSprite(DSWActor* breakActor, int type)
{
    if (TEST_BOOL1(breakActor))
    {
        if (TEST_BOOL2(breakActor))
            return false;

        return UserBreakSprite(breakActor);
    }

    if (breakActor->hasU() && !NullActor(breakActor))
    {
        // programmed animating type - without BOOL1 set
        if (breakActor->spr.lotag)
            DoLightingMatch(breakActor->spr.lotag, -1);

        SpawnShrap(breakActor, nullptr);
        breakActor->spr.extra &= ~SPRX_BREAKABLE;
        return false;
    }

    return AutoBreakSprite(breakActor, type);
}

void DoWallBreakMatch(int match)
{
    sectortype* sect = nullptr;
    int x,y,z;
    int wall_ang;

    for(auto& wal : wall)
    {
        if (wal.hitag == match)
        {
            WallBreakPosition(&wal, &sect, &x, &y, &z, &wall_ang);

            wal.hitag = 0; // Reset the hitag
            AutoBreakWall(&wal, x, y, z, wall_ang, 0);
        }
    }
}

static void DoWallBreakSpriteMatch(int match)
{
    SWStatIterator it(STAT_ENEMY);
    while (auto actor = it.Next())
    {
        if (actor->spr.hitag == match)
        {
            KillActor(actor);
        }
    }
}
END_SW_NS
