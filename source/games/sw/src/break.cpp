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
#include "buildtiles.h"


BEGIN_SW_NS

static void DoWallBreakSpriteMatch(int match);

BREAK_INFO WallBreakInfo[] =
{
    { IRONRAILING,  -1, SHRAP_METAL, BF_KILL, 0 },
    { NOTHING,  -1, SHRAP_METAL, BF_KILL, 0 },
    { METALGRATE, METALGRATEBROKE, SHRAP_METAL, 0, 0 },
    { COMPASSN, COMPASSBROKE, SHRAP_GLASS, 0, 0 },
    { COMPASSNW, COMPASSBROKE, SHRAP_GLASS, 0, 0 },
    { WINDOWGLASS, -1, SHRAP_GLASS, 0, 0 },
    { ARMATURES, ARMATURESBROKE, SHRAP_METAL, 0, 0 },
    { COMPDISPLAY, COMPDISPLAYBROKE, SHRAP_GLASS, 0, 0},
    { DC220, -1, SHRAP_METAL, 0, 0 },
    { MPHONE, MPHONEBROKE, SHRAP_METAL, 0, 0 },
    { LITEDISP1, LITEDISP1BROKE, SHRAP_METAL, 0, 0},
    { LITEDISP2, LITEDISP2BROKE, SHRAP_METAL, 0, 0},
    { LITEDISP3, LITEDISP3BROKE, SHRAP_GLASS, 0, 0},
    { LITEDISP4, LITEDISP3BROKE, SHRAP_GLASS, 0, 0 },
    { LITEDISP5, LITEDISP5BROKE, SHRAP_METAL, 0, 0},
    { LITEDISP6, LITEDISP6BROKE, SHRAP_METAL, 0, 0},
    { LITEDISP7, LITEDISP6BROKE, SHRAP_METAL, 0, 0},
    { LITEDISP8, LITEDISP6BROKE, SHRAP_METAL, 0, 0},
    { PAYPHONE, PAYPHONEBROKE, SHRAP_COIN, 0, 0},
    { PAYPHONE2, PAYPHONE2BROKE, SHRAP_COIN, 0, 0},
    { BIGLIGHT, LITEDISP1BROKE, SHRAP_GLASS, 0, 0},
    { DARKDISPLAY, LITEDISP1BROKE, SHRAP_GLASS, 0, 0},
    { TWOLIGHTS, TWOLIGHTSBROKE, SHRAP_GLASS, 0, 0},
    { DARKDISPLAY2, DARKDISPLAY2BROKE, SHRAP_GLASS, 0, 0},
    { VENDINGMACHINE, VENDINGMACHINEBROKE, SHRAP_GLASS, 0, 0},
    { FOURLIGHTS, FOURLIGHTSBROKE, SHRAP_GLASS, 0, 0},
    { FOURLIGHTSV, FOURLIGHTSVBROKE, SHRAP_GLASS, 0, 0},
    { VENDINGMACHINE2, VENDINGMACHINE2BROKE, SHRAP_GLASS, 0, 0},
    { METALGRATE2, METALGRATE2BROKE, SHRAP_METAL, 0, 0},
    { TWOLIGHTSV, TWOLIGHTSVBROKE, SHRAP_GLASS, 0, 0},
    { LITEDISP9, LITEDISP9BROKE, SHRAP_GLASS, 0, 0},
    { LITEDISP10, LITEDISP10BROKE, SHRAP_GLASS, 0, 0},
    { LITEDISP11, LITEDISP11BROKE, SHRAP_GLASS, 0, 0},
    { LITEDISP12, LITEDISP12BROKE, SHRAP_METAL, 0, 0},
    { CARFRONT, CARFRONTBROKE, SHRAP_METAL, 0, 0},
    { CARBACK, CARBACKBROKE, SHRAP_METAL, 0, 0},
    { CARHOOD, CARHOODBROKE, SHRAP_METAL, 0, 0},
    { CARSIDE, CARSIDEBROKE, SHRAP_METAL, 0, 0},
    { CARROOF, CARROOFBROKE, SHRAP_METAL, 0, 0},
    { CARWHEEL, CARWHEELBROKE, SHRAP_METAL, 0, 0},
    { ATMFRONT, ATMFRONTBROKE, SHRAP_COIN, 0, 0},
    { PAYPHONE3, PAYPHONE3BROKE, SHRAP_COIN, 0, 0},
    { CASHREGISTER, CASHREGISTER, SHRAP_METAL, 0, 0},
    { DRAWER, DRAWERBROKE, SHRAP_COIN, 0, 0},
    { COMPMONITOR, COMPMONITORBROKE, SHRAP_GLASS, 0, 0},
    { COMPMONITORSIDE, COMPMONITORSIDEBROKE, SHRAP_METAL, 0, 0},
    { PCFRONT, PCFRONTBROKE, SHRAP_METAL, 0, 0},
    { PCKEYNOARD, PCKEYNOARDBROKE, SHRAP_METAL, 0, 0},
    { VANBACK, VANBACKBROKE, SHRAP_METAL, 0, 0},
    { VANFRONT, VANFRONTBROKE, SHRAP_METAL, 0, 0},
    { LITEDISP13, LITEDISP13BROKE, SHRAP_METALMIX, 0, 0},
    { LITEDISP14, LITEDISP14BROKE, SHRAP_METALMIX, 0, 0},
    { LITEDISP15, LITEDISP15BROKE, SHRAP_METALMIX, 0, 0},
    { LITEDISP16, LITEDISP16BROKE, SHRAP_METALMIX, 0, 0},
    { LITEDISP17, LITEDISP17BROKE, SHRAP_METALMIX, 0, 0},
    { LITEDISP18, LITEDISP18BROKE, SHRAP_METALMIX, 0, 0},
    { LITEDISP19, LITEDISP19BROKE, SHRAP_METALMIX, 0, 0},
    { LITEDISP20, LITEDISP20BROKE, SHRAP_METALMIX, 0, 0},
    { LITEDISP21, LITEDISP21BROKE,  SHRAP_METALMIX, 0, 0},
    { GAMEROOMSIGN, GAMEROOMSIGNBROKE,  SHRAP_METALMIX, 0, 0},
    { PINBALL1, PINBALL1BROKE,  SHRAP_MARBELS, 0, 0},
    { PINBALL2, PINBALL2BROKE,  SHRAP_MARBELS, 0, 0},
    { PINBALL3, PINBALL3BROKE,  SHRAP_MARBELS, 0, 0},
    { PINBALL4, PINBALL4BROKE,  SHRAP_MARBELS, 0, 0},
    { DANGERSIGN, DANGERSIGNBROKE,  SHRAP_METAL, 0, 0},
    { LITEDISP22, LITEDISP22BROKE,  SHRAP_METAL, 0, 0},
    { TITSUBISHI, TITSUBISHIBROKE,  SHRAP_METAL, 0, 0},
    { ZILLACARSIDE, ZILLACARSIDEBROKE,  SHRAP_METAL, 0, 0},
};

BREAK_INFO SpriteBreakInfo[] =
{
    { IRONRAILING,  -1, SHRAP_METAL, BF_KILL, 0},
    { NOTHING,  -1, SHRAP_METAL, BF_KILL, 0},
    { BRA, -1, SHRAP_GENERIC, BF_KILL, 0},
    { COMPASSN, COMPASSBROKE, SHRAP_GLASS, 0, 0},
    { COMPASSNW, COMPASSBROKE, SHRAP_GLASS, 0, 0},
    { MURAL1, -1, SHRAP_PAPER, BF_BURN, 0},
    { MURAL2, -1, SHRAP_PAPER, BF_BURN, 0},
    { MURAL3, -1, SHRAP_WOOD, 0, 0},
    { MURAL4, -1, SHRAP_PAPER, BF_BURN, 0},
    { WINDOWGLASS, -1, SHRAP_GLASS, 0, 0},
    { ARMATURES, -1, SHRAP_METAL, 0, 0},
    { LITEDISP22, -1, SHRAP_METAL, 0, 0},
    { MACHINEDISP, -1, SHRAP_METAL, 0, 0},
    { HANGINGPOT, -1, SHRAP_METAL, BF_KILL, 0},
    { GHOSTLY, -1, SHRAP_STONE, BF_TOUGH|BF_KILL, 0},
    { HORNS, -1, SHRAP_METAL, BF_KILL,5},
    { SKELETON, -1, SHRAP_WOOD, BF_KILL, 0},
    { SKELETONB, -1, SHRAP_GIBS, BF_KILL, 0},
    { TREE10, -1, SHRAP_WOOD, BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { TREE20, -1, SHRAP_WOOD, BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { TREE30, -1, SHRAP_WOOD, BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { STONEHEAD, -1, SHRAP_STONE, BF_TOUGH|BF_KILL, 0},
    { STONEBUDDHA, -1, SHRAP_STONE, BF_TOUGH|BF_KILL, 0},
    { TREE40, -1, SHRAP_WOOD, BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { WEIRD1, -1, SHRAP_STONE, BF_KILL, 0},
    { TREE50, -1, SHRAP_WOOD, BF_KILL, 0},
    { COOKING1, -1, SHRAP_METAL, BF_KILL|BF_FIRE_FALL, 0},
    { BLOODYGIBS, -1, SHRAP_BLOOD, BF_KILL, 0},
    { HANGINGBODY1, -1, SHRAP_GIBS, BF_KILL, 0},
    { STABBEDINTHEHEAD, -1, SHRAP_GIBS, BF_KILL, 0},
    { STABBEDBODY1, -1, SHRAP_GIBS, BF_KILL|BF_TOUGH, 0},
    { HANGINGBODY2, -1, SHRAP_GIBS, BF_KILL, 0},
    { HANGINGBODY2a, -1, SHRAP_GIBS, BF_KILL, 0},
    { TREE60, -1, SHRAP_WOOD, BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { IMPALEDBODY, -1, SHRAP_GIBS, BF_KILL|BF_TOUGH, 0},
    { GUTTEDBODY, -1, SHRAP_GIBS, BF_KILL, 0},
    { FIRELAMP1, -1, SHRAP_METAL, BF_KILL|BF_FIRE_FALL, 0},
    { FIRELAMP2, -1, SHRAP_METAL, BF_KILL|BF_FIRE_FALL, 0},
    { FIRELAMP3, -1, SHRAP_METAL, BF_KILL|BF_FIRE_FALL, 0},
    { FIRELAMP4, -1, SHRAP_METAL, BF_KILL|BF_FIRE_FALL, 0},
    { PLANT10, -1, SHRAP_GENERIC, BF_KILL, 0},
    { TRUNK10, -1, SHRAP_WOOD, BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { TRUNK20, -1, SHRAP_WOOD, BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { DECO10, -1, SHRAP_METAL, BF_KILL, 0},
    { SYMBOL10, -1, SHRAP_PAPER, BF_BURN, 0},
    { SYMBOL20, -1, SHRAP_PAPER, BF_BURN, 0},
    { CANDLE10, -1, SHRAP_METAL, BF_KILL|BF_FIRE_FALL,5},
    { CANDLE20, -1, SHRAP_GENERIC, BF_KILL|BF_FIRE_FALL,5},
    { DRAGON10, -1, SHRAP_METAL, BF_KILL, 0},
    { CRAB10, -1, SHRAP_BLOOD, BF_KILL, 0},
    { CRAB11, -1, SHRAP_BLOOD, BF_KILL, 0},
    { CRAB12, -1, SHRAP_BLOOD, BF_KILL, 0},
    { BANNER10, -1, SHRAP_METAL, BF_BURN, 0},
    { FIRE10, -1, SHRAP_STONE, BF_KILL|BF_FIRE_FALL, 0},
    { FIRE11, -1, SHRAP_STONE, BF_KILL|BF_FIRE_FALL, 0},
    { FIRE12, -1, SHRAP_STONE, BF_KILL|BF_FIRE_FALL, 0},
    { FIRE13, -1, SHRAP_STONE, BF_KILL|BF_FIRE_FALL, 0},
    { FIRE14, -1, SHRAP_STONE, BF_KILL|BF_FIRE_FALL, 0},
    { FIRE15, -1, SHRAP_STONE, BF_KILL|BF_FIRE_FALL, 0},
    { FIRE16, -1, SHRAP_STONE, BF_KILL|BF_FIRE_FALL, 0},
    { FIRE17, -1, SHRAP_STONE, BF_KILL|BF_FIRE_FALL, 0},
    { IMPALEDBODY20, -1, SHRAP_GIBS, BF_KILL, 0},
    { SYMBOL30, -1, SHRAP_WOOD, BF_KILL, 0},
    { LAMP20OFF, -1, SHRAP_STONE, BF_KILL, 0},
    { LAMP20ON, -1, SHRAP_STONE, BF_KILL|BF_FIRE_FALL, 0},
    { WATERB, -1, SHRAP_WOOD, BF_KILL, 0},
    { CRYSTALS10, -1, SHRAP_STONE, BF_KILL, 0},
    { CRYSTALS20, -1, SHRAP_STONE, BF_KILL, 0},
    { BING10, -1, SHRAP_METAL, BF_KILL, 0},
    { BING11, -1, SHRAP_METAL, BF_KILL, 0},
    { BING12, -1, SHRAP_METAL, BF_KILL, 0},
    { POTTERY10, -1, SHRAP_STONE, BF_KILL, 0},
    { CRYSTALS30, -1, SHRAP_STONE, BF_KILL, 0},
    { THING10, -1, SHRAP_WOOD, BF_KILL, 0},
    { PBOTTLE, -1, SHRAP_GLASS, BF_KILL, 0},
    { CAUTIONWET, -1, SHRAP_GENERIC, BF_KILL, 0},
    { BLADETHING, -1, SHRAP_WOOD, BF_KILL, 0},
    { THING20, -1, SHRAP_WOOD, BF_KILL|BF_TOUGH, 0},
    { LAMP30, -1, SHRAP_WOOD, BF_KILL, 0},
    { WALLSWORDS, -1, SHRAP_METAL, BF_KILL, 0},
    { OFFICECHAIR, -1, SHRAP_METAL, BF_KILL, 0},
    { THING30, -1, SHRAP_GENERIC, BF_KILL, 0},
    { FOODPLATE10, -1, SHRAP_METAL, BF_KILL, 0},
    { FOODPLATE20, -1, SHRAP_METAL, BF_KILL, 0},
    { OFFICELAMP, -1, SHRAP_METAL, BF_KILL,5},
    { REDLIGHT10, -1, SHRAP_BLOOD, BF_KILL,5},
    { YELLIGHT10, -1, SHRAP_GENERIC, BF_KILL,5},
    { WHISKEYB, -1, SHRAP_GLASS, BF_KILL, 0},
    { FOODBOWL, -1, SHRAP_GLASS, BF_KILL, 0},
    { CUP10, -1, SHRAP_GLASS, BF_KILL, 0},
    { CUP20, -1, SHRAP_GLASS, BF_KILL, 0},
    { COOKPOT10, -1, SHRAP_METAL, BF_KILL, 0},
    { FIRE20, -1, SHRAP_METAL, BF_KILL|BF_FIRE_FALL,5},
    { SHRUB10, -1, SHRAP_WOOD, BF_BURN, 0},
    { SHRUB20, -1, SHRAP_WOOD, BF_BURN, 0},
    { SHRUB30, -1, SHRAP_WOOD, BF_BURN, 0},
    { LAMP40, -1, SHRAP_WOOD, BF_FIRE_FALL|BF_KILL,5},
    { LAMP50, -1, SHRAP_WOOD, BF_KILL, 0},
    { FIRE30, -1, SHRAP_WOOD, BF_KILL|BF_FIRE_FALL, 0},
    { LAMP60, -1, SHRAP_METAL, BF_KILL|BF_FIRE_FALL, 0},
    { FIREBLUE, -1, SHRAP_WOOD, BF_KILL|BF_FIRE_FALL, 0},
    { ROTFAN, -1, SHRAP_METAL, BF_KILL, 0},
    { COOKPOT20, -1, SHRAP_METAL, BF_KILL, 0},
    { CAN20, -1, SHRAP_METAL, BF_KILL, 0},
    { EMPTY10, EMPTY10BROKE, SHRAP_GLASS,BF_TOUGH, 0},
    { WASHTUB, -1, SHRAP_METAL, BF_KILL, 0},
    { HBOTTLE10, -1, SHRAP_LARGE_EXPLOSION, BF_KILL, 0},
    { CHAIR10, -1, SHRAP_METAL, BF_KILL, 0},
    { ROTFAN20, -1, SHRAP_METAL, BF_KILL, 0},
    { ALERTLIGHT, -1, SHRAP_GLASS, BF_KILL, 0},
    { HBOTTLE20, -1, SHRAP_LARGE_EXPLOSION, BF_KILL, 0},
    { HBOTTLE30, -1, SHRAP_LARGE_EXPLOSION, BF_KILL, 0},
    { CRANEHOOK, -1, SHRAP_METAL, BF_KILL, 0},
    { CAN30, -1, SHRAP_METAL, BF_KILL, 0},
    { SCREWDRIVER, -1, SHRAP_METAL, BF_KILL, 0},
    { BARREL10, -1, SHRAP_LARGE_EXPLOSION, BF_KILL, 0},
    { BLACKSTUFF, -1, SHRAP_METAL, BF_KILL,5},
    { RACK10, -1, SHRAP_METAL, BF_KILL,10},
    { SCREWY, -1, SHRAP_METAL, BF_KILL,5},
    { SHROOM10, -1, SHRAP_PLANT, BF_KILL, 0},
    { LAMP80, -1, SHRAP_METAL, BF_KILL|BF_FIRE_FALL, 0},
    { LAMP90, -1, SHRAP_GLASS, BF_KILL|BF_FIRE_FALL, 0},
    { CHEMBULB, -1, SHRAP_BLOOD, BF_KILL, 0},
    { REDBODY, -1, SHRAP_GIBS, BF_KILL, 0},
    { REDBODY2, -1, SHRAP_GIBS, BF_KILL, 0},
    { REDBODY3, -1, SHRAP_GIBS, BF_KILL, 0},
    { BOWL20, -1, SHRAP_GLASS, BF_KILL,5},
    { REDALERT, -1, SHRAP_GLASS, BF_KILL,5},
    { MAGIC10, -1, SHRAP_GLASS, BF_KILL,5},
    { CHEMBULB2, -1, SHRAP_GLASS, BF_KILL,5},
    { BLUEBOTTLE, -1, SHRAP_GLASS, BF_KILL,5},
    { BUCKET10, -1, SHRAP_PAPER, BF_KILL,5},
    { ROCK10, -1, SHRAP_STONE, BF_KILL|BF_TOUGH, 0},
    { ROCK20, -1, SHRAP_STONE, BF_KILL|BF_TOUGH, 0},
    { TREE70, -1, SHRAP_WOOD, BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { TREE80, -1, SHRAP_WOOD, BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { ROCK30, -1, SHRAP_STONE, BF_KILL, 0},
    { ROCK40, -1, SHRAP_STONE, BF_KILL, 0},
    { ROCK50, -1, SHRAP_STONE, BF_KILL, 0},
    { ROCK60, -1, SHRAP_STONE, BF_KILL, 0},
    { STATUE20, -1, SHRAP_STONE, BF_TOUGH|BF_KILL, 0},
    { STATUEUPPER, -1, SHRAP_STONE, BF_TOUGH|BF_KILL, 0},
    { STATUE30, -1, SHRAP_STONE, BF_TOUGH|BF_KILL, 0},
    { STATUE40, -1, SHRAP_STONE, BF_TOUGH|BF_KILL, 0},
    { UWATER10, -1, SHRAP_PLANT, BF_KILL, 0},
    { UWATER20, -1, SHRAP_PLANT, BF_KILL, 0},
    { UWATER30, -1, SHRAP_PLANT, BF_KILL, 0},
    { UWATER40, -1, SHRAP_PLANT, BF_KILL, 0},
    { UWATER50, -1, SHRAP_STONE, BF_KILL,5},
    { MESSY10, -1, SHRAP_GIBS, BF_KILL, 0},
    { MESSY11, -1, SHRAP_GIBS, BF_KILL, 0},
    { MESSY12, -1, SHRAP_GIBS, BF_KILL, 0},
    { MESSY13, -1, SHRAP_GIBS, BF_KILL, 0},
    { UWATER60, -1, SHRAP_PLANT, BF_KILL, 0},
    { UWATER70, -1, SHRAP_GENERIC, BF_KILL, 0},
    { FLOORPLAN, -1, SHRAP_GLASS, BF_KILL,5},
    { PICTURE10, -1, SHRAP_PAPER, BF_BURN, 0},
    { PICTURE20, -1, SHRAP_PAPER, BF_BURN, 0},
    { TREE100, -1, SHRAP_WOOD, BF_TOUGH|BF_BURN, 0},
    { TREE110, -1, SHRAP_WOOD, BF_TOUGH|BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { TREE120, -1, SHRAP_WOOD, BF_TOUGH|BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { TREE130, -1, SHRAP_WOOD, BF_TOUGH|BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { TREE140, -1, SHRAP_WOOD, BF_TOUGH|BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { TREE150, -1, SHRAP_WOOD, BF_TOUGH|BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { TREE160, -1, SHRAP_WOOD, BF_TOUGH|BF_BURN|BF_OVERRIDE_BLOCK, 0},
    { TREE170, -1, SHRAP_WOOD, BF_TOUGH|BF_KILL, 0},
    { TREE180, -1, SHRAP_WOOD, BF_TOUGH|BF_KILL, 0},
    { TREE190, -1, SHRAP_WOOD, BF_TOUGH|BF_KILL, 0},
    { TREE200, -1, SHRAP_WOOD, BF_TOUGH|BF_KILL, 0},
    { TREE210, -1, SHRAP_WOOD, BF_TOUGH|BF_BURN, 0},
    { TREE220, -1, SHRAP_WOOD, BF_TOUGH|BF_BURN, 0},
    { CRYSTALS40, -1, SHRAP_STONE, BF_KILL,5},
    { CRYSTALS50, -1, SHRAP_STONE, BF_KILL, 0},
    { CRYSTALS60, -1, SHRAP_STONE, BF_KILL, 0},
    { WEIRD20   , -1, SHRAP_GENERIC, BF_KILL, 0},
    { SUITCASE10, -1, SHRAP_GENERIC, BF_KILL, 0},
    { SUITCASE20, -1, SHRAP_GENERIC, BF_KILL, 0},
    { SUITCASE30, -1, SHRAP_GENERIC, BF_KILL, 0},
    { SUITCASE40, -1, SHRAP_GENERIC, BF_KILL, 0},
    { SUITCASE50, -1, SHRAP_GENERIC, BF_KILL, 0},
    { FOURLIGHTS, FOURLIGHTSBROKE, SHRAP_GLASS, 0, 0},
    { FOURLIGHTSV, FOURLIGHTSVBROKE, SHRAP_GLASS, 0, 0},
    { NOTHING20      , -1, SHRAP_WOOD, BF_KILL, 0},
    { CART           , -1, SHRAP_WOOD, BF_TOUGH|BF_KILL, 0},
    { BURNINGCORPSE10, -1, SHRAP_GIBS, BF_KILL, 0},
    { BURNINGCORPSE11, -1, SHRAP_GIBS, BF_KILL, 0},
    { BURNINGCORPSE12, -1, SHRAP_GIBS, BF_KILL, 0},
    { BURNINGCORPSE13, -1, SHRAP_GIBS, BF_KILL, 0},
    { DEADTHING10    , -1, SHRAP_GIBS, BF_KILL, 0},
    { GIBS10         , -1, SHRAP_BLOOD, BF_KILL, 0},
    { GIBS20         , -1, SHRAP_BLOOD, BF_KILL, 0},
    { CHINESE10      , -1, SHRAP_GLASS, BF_KILL, 0},
    { CHINESE20      , -1, SHRAP_WOOD, BF_KILL, 0},
    { DARKDISPLAY2, DARKDISPLAY2BROKE, SHRAP_GLASS, 0, 0},
    { CEILLITE, CEILLITEBROKE, SHRAP_GLASS, 0, 0},
    { METALGRATE2, METALGRATE2BROKE, SHRAP_METAL, BF_TOUGH, 0},
    { TWOLIGHTSV, TWOLIGHTSVBROKE, SHRAP_GLASS, 0, 0},
    { WALLCLOCK, WALLCLOCKBROKE, SHRAP_METAL, BF_TOUGH|BF_KILL, 0},
    { NOTEBLOCK    , -1, SHRAP_PAPER, BF_KILL, 0 },
    { TOWELBLUE    , -1, SHRAP_GENERIC, BF_KILL | BF_TOUGH, 0 },
    { CHAINS10     , -1, SHRAP_METAL, BF_KILL, 0 },
    { CHAINS20     , -1, SHRAP_METAL, BF_KILL, 0 },
    { CHAINS30     , -1, SHRAP_METAL, BF_KILL, 0 },
    { CHAINS40     , -1, SHRAP_METAL, BF_KILL, 0 },
    { NOTES        , -1, SHRAP_METAL, BF_KILL, 0 },
    { BUCKET20     , -1, SHRAP_METAL, BF_KILL, 0 },
    { GREENTHING   , -1, SHRAP_METAL, BF_KILL, 0 },
    { WODDENTUB    , -1, SHRAP_WOOD, BF_KILL, 0 },
    { TREE230      , -1, SHRAP_WOOD, BF_BURN, 0 },
    { GOLDCUP      , -1, SHRAP_METAL, BF_KILL, 0 },
    { METALTHING   , -1, SHRAP_METAL, BF_KILL, 0 },
    { SHRUB40      , -1, SHRAP_WOOD, BF_TOUGH | BF_BURN | BF_OVERRIDE_BLOCK, 0 },
    { CHINESE30    , -1, SHRAP_PAPER, BF_KILL, 0 },
    { CHINESE40    , -1, SHRAP_PAPER, BF_KILL, 0 },
    { CHINESE50    , -1, SHRAP_PAPER, BF_KILL, 0 },
    { HALBERDS     , -1, SHRAP_METAL, BF_KILL, 0 },
    { WALLSWORD2   , -1, SHRAP_METAL, BF_KILL, 0 },
    { DEADTURTLE   , -1, SHRAP_GIBS, BF_KILL, 0 },
    { PARKINGMETER1, - 1, SHRAP_COIN,BF_KILL, 0 },
    { PARKINGMETER2, - 1, SHRAP_COIN,BF_KILL, 0 },
    { LANTERN1     , -1, SHRAP_PAPER,BF_KILL, 0 },
    { LANTERN2     , -1, SHRAP_PAPER,BF_KILL, 0 },
    { LANTERN3     , -1, SHRAP_PAPER,BF_KILL, 0 },
    { LANTERN4     , -1, SHRAP_PAPER,BF_KILL, 0 },
    { LANTERN5     , -1, SHRAP_PAPER,BF_KILL, 0 },
    { LANTERN6     , -1, SHRAP_PAPER,BF_KILL, 0 },
    { LAMP70, LAMP70BROKE, SHRAP_METAL,BF_TOUGH | BF_KILL, 0 },
    { TRASHCAN1 , -1, SHRAP_METAL,BF_TOUGH | BF_KILL, 0 },
    { POLE10    , -1, SHRAP_METAL,BF_KILL, 0 },
    { BLACKBEAM , -1, SHRAP_PAPER,BF_KILL, 0 },
    { RECTSCREEN, -1, SHRAP_GLASS,BF_KILL, 0 },
    { POLE20    , -1,  SHRAP_METAL,BF_TOUGH | BF_KILL, 0 },
    { CAN50     , -1,  SHRAP_METAL,BF_KILL, 0 },
    { SCREWY2   , -1,  SHRAP_METAL,BF_KILL, 0 },
    { SCREWY3   , -1,  SHRAP_METAL,BF_KILL, 0 },
    { GRAYSTUFF , -1,  SHRAP_METAL,BF_KILL, 0 },
    { LARACROFT ,-1,  SHRAP_GIBS, BF_KILL, 0 },
    { POWDERKEG1, -1,  SHRAP_LARGE_EXPLOSION, BF_KILL, 0 },
    { POWDERKEG2, -1,  SHRAP_LARGE_EXPLOSION, BF_KILL, 0 },
    { POWDERKEG3, -1,  SHRAP_WOODMIX, BF_KILL | BF_TOUGH, 0 },
    { SHARKFIN  ,-1,  SHRAP_GIBS, BF_KILL, 0},
    { DEADGAME,DEADGAMEBROKE,SHRAP_GIBS, BF_TOUGH|BF_KILL, 0},
    { DEADGAMEBROKE,-1,  SHRAP_WOOD, BF_TOUGH|BF_KILL, 0},
    { PARASOL,PARASOLBROKE,SHRAP_WOOD, BF_TOUGH|BF_KILL, 0},
    { PARASOLBROKE,-1,  SHRAP_WOOD, BF_TOUGH|BF_KILL, 0},
    { HEART10      ,-1,  SHRAP_GIBS, BF_KILL, 0},
    { HEART20      ,-1,  SHRAP_GIBS, BF_KILL, 0},
    { HEART30      ,-1,  SHRAP_GIBS, BF_KILL, 0},
    { LIVER10      ,-1,  SHRAP_GIBS, BF_KILL, 0},
    { LIVER20      ,-1,  SHRAP_GIBS, BF_KILL, 0},
    { LIVER30      ,-1,  SHRAP_GIBS, BF_KILL, 0},
    { GIBBED10     ,-1,  SHRAP_GIBS, BF_KILL, 0},
    { GIBBED20     ,-1,  SHRAP_GIBS, BF_KILL, 0},
    { GIBBED30     ,-1,  SHRAP_GIBS, BF_KILL, 0},
    { BLOODSKULL30,-1,  SHRAP_GIBS, BF_KILL, 0},
    { CALTROPS2   ,-1,  SHRAP_METAL,BF_KILL|BF_TOUGH, 0},
    { FLY10       ,-1,  SHRAP_GLASS,BF_KILL, 0},
    { FLY20       ,-1,  SHRAP_GLASS,BF_KILL, 0},
    { FLY30       ,-1,  SHRAP_GLASS,BF_KILL, 0},
    { FLY40       ,-1,  SHRAP_GLASS,BF_KILL, 0},
    { TRAININGDEV ,-1,  SHRAP_WOOD, BF_KILL|BF_TOUGH, 0},
    { TRAININGDOLL,TRAININGDOLLBROKE,SHRAP_PAPERMIX, BF_KILL|BF_TOUGH|BF_LEAVE_BREAK, 0},
    { TRAININGDOLLBROKE,-1,  SHRAP_PAPERMIX, BF_KILL|BF_TOUGH, 0},
    { CRYBABY      ,-1,  SHRAP_GIBS, BF_KILL, 0},
    { NASTERLEEP   ,-1,   SHRAP_GIBS, BF_KILL, 0},
    { LOWANGDEAD   , -1, SHRAP_GIBS, BF_KILL | BF_TOUGH, 0 }, // old ninja dead frames
    { BLOODSKULL   , -1, SHRAP_GIBS, BF_KILL | BF_TOUGH, 0 },

    // below are breakable death sprites of enemies etc. (All enemies except the bosses leave breakable corpses!)
    { BUNNY_DEAD   ,-1,  SHRAP_GIBS, BF_KILL, 0 },
    { COOLG_DEAD    , -1, SHRAP_GIBS, BF_KILL | BF_TOUGH, 0 },  // Dead Coolie Ghost
    { COOLIE_DEAD_NOHEAD      , -1, SHRAP_GIBS, BF_KILL | BF_TOUGH, 0 },
    { EEL_DEAD     ,-1,  SHRAP_GIBS, BF_KILL, 0 },
    { GIRLNINJA_DEAD,-1,  SHRAP_GIBS, BF_KILL | BF_TOUGH, 0 },
    { GORO_DEAD  , -1, SHRAP_GIBS, BF_KILL | BF_TOUGH, 0 },
    { HORNET_DEAD - 1, SHRAP_GIBS, BF_KILL | BF_TOUGH, 0 }, // dead actors
    { NINJA_HARAKIRI_DEAD    , -1, SHRAP_GIBS, BF_KILL | BF_TOUGH, 0 }, // Evil ninja Hari-Kari - can gib
    { NINJA_DEAD1     , -1, SHRAP_GIBS, BF_KILL | BF_TOUGH, 0 },
    { NINJA_DEAD_SLICED, -1, SHRAP_GIBS, BF_KILL | BF_TOUGH, 0 },  // Evil Ninja cut in half
    { PLAYER_NINJA_DEAD  ,-1,  SHRAP_GIBS, BF_KILL | BF_TOUGH, 0 },
    { NINJA_HeadFly   , -1, SHRAP_GIBS, BF_KILL | BF_TOUGH, 0 },
    { RIPPER_DEAD    , -1, SHRAP_GIBS, BF_KILL | BF_TOUGH, 0 },
    { RIPPER2_DEAD   , -1, SHRAP_GIBS, BF_KILL | BF_TOUGH, 0 },  // Dead Big Ripper

    { CALTROPS     , -1, SHRAP_METAL,BF_KILL, 0 },   // Caltrops are breakable


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

int CompareSearchBreakInfo(int* picnum, BREAK_INFO* break_info)
{
    // will return a number less than 0 if picnum < break_info->picnum
    return(*picnum - break_info->picnum);
}

BREAK_INFO* FindWallBreakInfo(FTextureID texid)
{
    int picnum = legacyTileNum(texid);
    return(BREAK_INFO*)(bsearch(&picnum, &WallBreakInfo, SIZ(WallBreakInfo), sizeof(BREAK_INFO), (int(*)(const void*, const void*))CompareSearchBreakInfo));
}

BREAK_INFO* FindSpriteBreakInfo(int picnum)
{
    return(BREAK_INFO*)(bsearch(&picnum, &SpriteBreakInfo, SIZ(SpriteBreakInfo), sizeof(BREAK_INFO), (int(*)(const void*, const void*))CompareSearchBreakInfo));
}

//////////////////////////////////////////////
// SETUP
//////////////////////////////////////////////

void SortBreakInfo(void)
{
    qsort(&SpriteBreakInfo, SIZ(SpriteBreakInfo), sizeof(BREAK_INFO), CompareBreakInfo);
    qsort(&WallBreakInfo, SIZ(WallBreakInfo), sizeof(BREAK_INFO), CompareBreakInfo);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

BREAK_INFO* SetupWallForBreak(walltype* wallp)
{
    BREAK_INFO* break_info;

    break_info = FindWallBreakInfo(wallp->walltexture);
    if (break_info)
    {
        wallp->lotag = TAG_WALL_BREAK;
        wallp->extra |= (WALLFX_DONT_STICK);
    }

    if (wallp->overtexture.isValid() && (wallp->cstat & CSTAT_WALL_MASKED))
    {
        break_info = FindWallBreakInfo(wallp->overtexture);
        if (break_info)
        {
            wallp->lotag = TAG_WALL_BREAK;
            wallp->extra |= (WALLFX_DONT_STICK);
        }
    }

    return break_info;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

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

        SetActorSizeX(actor);

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

//---------------------------------------------------------------------------
//
// WALL
//
//---------------------------------------------------------------------------

int AutoBreakWall(walltype* wallp, const DVector3& hit_pos, DAngle ang, int type)
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
            nwp->overtexture.isValid() &&
            (nwp->cstat & CSTAT_WALL_MASKED))
        {
            nwp->lotag = 0;
        }
    }

    if (wallp->overtexture.isValid() && (wallp->cstat & CSTAT_WALL_MASKED))
        break_info = FindWallBreakInfo(wallp->overtexture);
    else
        break_info = FindWallBreakInfo(wallp->walltexture);

    if (!break_info)
    {
        return false;
    }

    // Check to see if it should break with current weapon type
    if (!CheckBreakToughness(break_info, type)) return false;

    if (hit_pos.X != INT32_MAX)
    {
        // need correct location for spawning shrap
        auto breakActor = insertActor(0, STAT_DEFAULT);
        breakActor->spr.cstat = 0;
        breakActor->spr.extra = 0;
        breakActor->spr.Angles.Yaw = ang;
        breakActor->spr.picnum = ST1;
        breakActor->spr.scale = DVector2(1, 1);
        SetActorZ(breakActor, hit_pos);
        SpawnShrap(breakActor, nullptr, -1, break_info);
        KillActor(breakActor);
    }

    // change the wall
    if (wallp->overtexture.isValid() && (wallp->cstat & CSTAT_WALL_MASKED))
    {
        if (break_info->breaknum == -1)
        {
            wallp->cstat &= ~(CSTAT_WALL_MASKED|CSTAT_WALL_1WAY|CSTAT_WALL_BLOCK_HITSCAN|CSTAT_WALL_BLOCK);
            wallp->setovertexture(FNullTextureID());
            if (wallp->twoSided())
            {
                nwp = wallp->nextWall();
                nwp->cstat &= ~(CSTAT_WALL_MASKED|CSTAT_WALL_1WAY|CSTAT_WALL_BLOCK_HITSCAN|CSTAT_WALL_BLOCK);
                nwp->setovertexture(FNullTextureID());
            }
        }
        else
        {
            wallp->cstat &= ~(CSTAT_WALL_BLOCK_HITSCAN|CSTAT_WALL_BLOCK);
            wallp->setovertexture(tileGetTextureID(break_info->breaknum));
            if (wallp->twoSided())
            {
                nwp = wallp->nextWall();
                nwp->cstat &= ~(CSTAT_WALL_BLOCK_HITSCAN|CSTAT_WALL_BLOCK);
                nwp->setovertexture(tileGetTextureID(break_info->breaknum));
            }
        }
    }
    else
    {
        if (break_info->breaknum == -1)
            wallp->setwalltexture(FNullTextureID()); // temporary break pic
        else
        {
            wallp->setwalltexture(tileGetTextureID(break_info->breaknum));
            if (wallp->hitag < 0)
                DoWallBreakSpriteMatch(wallp->hitag);
        }
    }
    return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

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

    if (wp->walltexture == actor->texparam)
        return true;

    // make it BROKEN
    if (SP_TAG7(actor) <= 1)
    {
        DoSpawnSpotsForKill(match);
        DoLightingMatch(match, -1);

        if (SP_TAG8(actor) == 0)
        {
            wp->setwalltexture(actor->texparam);
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
            wp->setwalltexture(actor->texparam);

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
        wp->setwalltexture(wp->walltexture + 1);

        DoSpawnSpotsForDamage(match);
    }

    return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int WallBreakPosition(walltype* wp, sectortype** sectp, DVector3& pos, DAngle& ang)
{
    int nx,ny;
    DAngle wall_ang = wp->delta().Angle() + DAngle90;

    *sectp = wp->sectorp();
    ASSERT(*sectp);

    // midpoint of wall
    pos.XY() = wp->center();

    if (!wp->twoSided())
    {
        // white wall
        pos.Z = ((*sectp)->floorz + (*sectp)->ceilingz) * 0.5;
    }
    else
    {
        auto next_sect = wp->nextSector();

        // red wall
        ASSERT(wp->twoSided());

        // floor and ceiling meet
        if (next_sect->floorz == next_sect->ceilingz)
            pos.Z = ((*sectp)->floorz + (*sectp)->ceilingz) * 0.5;
        else
        // floor is above other sector
        if (next_sect->floorz < (*sectp)->floorz)
            pos.Z = (next_sect->floorz + (*sectp)->floorz) * 0.5;
        else
        // ceiling is below other sector
        if (next_sect->ceilingz > (*sectp)->ceilingz)
            pos.Z = (next_sect->ceilingz + (*sectp)->ceilingz) * 0.5;
    }

    ang = wall_ang;

    pos.XY() += wall_ang.ToVector() * 16;

    updatesectorz(pos, sectp);
    if (*sectp == nullptr)
    {
        pos.X = INT32_MAX;  // don't spawn shrap, just change wall
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------
//
// If the tough parameter is not set, then it can't break tough walls and sprites
//
//---------------------------------------------------------------------------

bool HitBreakWall(walltype* wp, DVector3 hitpos, DAngle ang, int type)
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
        WallBreakPosition(wp, &sect, hitpos, ang);
    }

    AutoBreakWall(wp, hitpos, ang, type);
    return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void DoWallBreakMatch(int match)
{
    sectortype* sect = nullptr;
    DVector3 hitpos;
    DAngle wall_ang;

    for(auto& wal : wall)
    {
        if (wal.hitag == match)
        {
            WallBreakPosition(&wal, &sect, hitpos, wall_ang);

            wal.hitag = 0; // Reset the hitag
            AutoBreakWall(&wal, hitpos, wall_ang, 0);
        }
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

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
