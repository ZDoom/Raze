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
#include "ai.h"

BEGIN_BLD_NS

extern AISTATE batIdle;
extern AISTATE batFlyIdle;
extern AISTATE batChase;
extern AISTATE batPonder;
extern AISTATE batGoto;
extern AISTATE batBite;
extern AISTATE batRecoil;
extern AISTATE batSearch;
extern AISTATE batSwoop;
extern AISTATE batFly;
extern AISTATE batTurn;
extern AISTATE batHide;
extern AISTATE batDodgeUp;
extern AISTATE batDodgeUpRight;
extern AISTATE batDodgeUpLeft;
extern AISTATE batDodgeDown;
extern AISTATE batDodgeDownRight;
extern AISTATE batDodgeDownLeft;

extern AISTATE beastIdle;
extern AISTATE beastChase;
extern AISTATE beastDodge;
extern AISTATE beastGoto;
extern AISTATE beastSlash;
extern AISTATE beastStomp;
extern AISTATE beastSearch;
extern AISTATE beastRecoil;
extern AISTATE beastTeslaRecoil;
extern AISTATE beastSwimIdle;
extern AISTATE beastSwimChase;
extern AISTATE beastSwimDodge;
extern AISTATE beastSwimGoto;
extern AISTATE beastSwimSearch;
extern AISTATE beastSwimSlash;
extern AISTATE beastSwimRecoil;
extern AISTATE beastMorphToBeast;
extern AISTATE beastMorphFromCultist;
extern AISTATE beast138FB4;
extern AISTATE beast138FD0;
extern AISTATE beast138FEC;

extern AISTATE eelIdle;
extern AISTATE eelFlyIdle;
extern AISTATE eelChase;
extern AISTATE eelPonder;
extern AISTATE eelGoto;
extern AISTATE eelBite;
extern AISTATE eelRecoil;
extern AISTATE eelSearch;
extern AISTATE eelSwoop;
extern AISTATE eelFly;
extern AISTATE eelTurn;
extern AISTATE eelHide;
extern AISTATE eelDodgeUp;
extern AISTATE eelDodgeUpRight;
extern AISTATE eelDodgeUpLeft;
extern AISTATE eelDodgeDown;
extern AISTATE eelDodgeDownRight;
extern AISTATE eelDodgeDownLeft;

extern AISTATE cultistBurnIdle;
extern AISTATE cultistBurnChase;
extern AISTATE cultistBurnGoto;
extern AISTATE cultistBurnSearch;
extern AISTATE cultistBurnAttack;
extern AISTATE zombieABurnChase;
extern AISTATE zombieABurnGoto;
extern AISTATE zombieABurnSearch;
extern AISTATE zombieABurnAttack;
extern AISTATE zombieFBurnChase;
extern AISTATE zombieFBurnGoto;
extern AISTATE zombieFBurnSearch;
extern AISTATE zombieFBurnAttack;
extern AISTATE innocentBurnChase;
extern AISTATE innocentBurnGoto;
extern AISTATE innocentBurnSearch;
extern AISTATE innocentBurnAttack;
extern AISTATE beastBurnChase;
extern AISTATE beastBurnGoto;
extern AISTATE beastBurnSearch;
extern AISTATE beastBurnAttack;
extern AISTATE tinycalebBurnChase;
extern AISTATE tinycalebBurnGoto;
extern AISTATE tinycalebBurnSearch;
extern AISTATE tinycalebBurnAttack;
extern AISTATE genDudeBurnIdle;
extern AISTATE genDudeBurnChase;
extern AISTATE genDudeBurnGoto;
extern AISTATE genDudeBurnSearch;
extern AISTATE genDudeBurnAttack;

extern AISTATE tinycalebIdle;
extern AISTATE tinycalebChase;
extern AISTATE tinycalebDodge;
extern AISTATE tinycalebGoto;
extern AISTATE tinycalebAttack;
extern AISTATE tinycalebSearch;
extern AISTATE tinycalebRecoil;
extern AISTATE tinycalebTeslaRecoil;
extern AISTATE tinycalebSwimIdle;
extern AISTATE tinycalebSwimChase;
extern AISTATE tinycalebSwimDodge;
extern AISTATE tinycalebSwimGoto;
extern AISTATE tinycalebSwimSearch;
extern AISTATE tinycalebSwimAttack;
extern AISTATE tinycalebSwimRecoil;
extern AISTATE tinycaleb139660;
extern AISTATE tinycaleb13967C;
extern AISTATE tinycaleb139698;

extern AISTATE cerberusIdle;
extern AISTATE cerberusSearch;
extern AISTATE cerberusChase;
extern AISTATE cerberusRecoil;
extern AISTATE cerberusTeslaRecoil;
extern AISTATE cerberusGoto;
extern AISTATE cerberusBite;
extern AISTATE cerberusBurn;
extern AISTATE cerberus3Burn;
extern AISTATE cerberus2Idle;
extern AISTATE cerberus2Search;
extern AISTATE cerberus2Chase;
extern AISTATE cerberus2Recoil;
extern AISTATE cerberus2Goto;
extern AISTATE cerberus2Bite;
extern AISTATE cerberus2Burn;
extern AISTATE cerberus4Burn;
extern AISTATE cerberus139890;
extern AISTATE cerberus1398AC;

extern AISTATE cultistIdle;
extern AISTATE cultistProneIdle;
extern AISTATE fanaticProneIdle;
extern AISTATE cultistProneIdle3;
extern AISTATE cultistChase;
extern AISTATE fanaticChase;
extern AISTATE cultistDodge;
extern AISTATE cultistGoto;
extern AISTATE cultistProneChase;
extern AISTATE cultistProneDodge;
extern AISTATE cultistTThrow;
extern AISTATE cultistSThrow;
extern AISTATE cultistTsThrow;
extern AISTATE cultistDThrow;
extern AISTATE cultist139A78;
extern AISTATE cultist139A94;
extern AISTATE cultist139AB0;
extern AISTATE cultist139ACC;
extern AISTATE cultist139AE8;
extern AISTATE cultistSearch;
extern AISTATE cultistSFire;
extern AISTATE cultistTFire;
extern AISTATE cultistTsFire;
extern AISTATE cultistSProneFire;
extern AISTATE cultistTProneFire;
extern AISTATE cultistTsProneFire;
extern AISTATE cultistRecoil;
extern AISTATE cultistProneRecoil;
extern AISTATE cultistTeslaRecoil;
extern AISTATE cultistSwimIdle;
extern AISTATE cultistSwimChase;
extern AISTATE cultistSwimDodge;
extern AISTATE cultistSwimGoto;
extern AISTATE cultistSwimSearch;
extern AISTATE cultistSSwimFire;
extern AISTATE cultistTSwimFire;
extern AISTATE cultistTsSwimFire;
extern AISTATE cultistSwimRecoil;

extern AISTATE gargoyleFIdle;
extern AISTATE gargoyleStatueIdle;
extern AISTATE gargoyleFChase;
extern AISTATE gargoyleFGoto;
extern AISTATE gargoyleFSlash;
extern AISTATE gargoyleFThrow;
extern AISTATE gargoyleSThrow;
extern AISTATE gargoyleSBlast;
extern AISTATE gargoyleFRecoil;
extern AISTATE gargoyleFSearch;
extern AISTATE gargoyleFMorph2;
extern AISTATE gargoyleFMorph;
extern AISTATE gargoyleSMorph2;
extern AISTATE gargoyleSMorph;
extern AISTATE gargoyleSwoop;
extern AISTATE gargoyleFly;
extern AISTATE gargoyleTurn;
extern AISTATE gargoyleDodgeUp;
extern AISTATE gargoyleFDodgeUpRight;
extern AISTATE gargoyleFDodgeUpLeft;
extern AISTATE gargoyleDodgeDown;
extern AISTATE gargoyleFDodgeDownRight;
extern AISTATE gargoyleFDodgeDownLeft;
extern AISTATE statueFBreakSEQ;
extern AISTATE statueSBreakSEQ;

extern AISTATE ghostIdle;
extern AISTATE ghostChase;
extern AISTATE ghostGoto;
extern AISTATE ghostSlash;
extern AISTATE ghostThrow;
extern AISTATE ghostBlast;
extern AISTATE ghostRecoil;
extern AISTATE ghostTeslaRecoil;
extern AISTATE ghostSearch;
extern AISTATE ghostSwoop;
extern AISTATE ghostFly;
extern AISTATE ghostTurn;
extern AISTATE ghostDodgeUp;
extern AISTATE ghostDodgeUpRight;
extern AISTATE ghostDodgeUpLeft;
extern AISTATE ghostDodgeDown;
extern AISTATE ghostDodgeDownRight;
extern AISTATE ghostDodgeDownLeft;

extern AISTATE gillBeastIdle;
extern AISTATE gillBeastChase;
extern AISTATE gillBeastDodge;
extern AISTATE gillBeastGoto;
extern AISTATE gillBeastBite;
extern AISTATE gillBeastSearch;
extern AISTATE gillBeastRecoil;
extern AISTATE gillBeastSwimIdle;
extern AISTATE gillBeastSwimChase;
extern AISTATE gillBeastSwimDodge;
extern AISTATE gillBeastSwimGoto;
extern AISTATE gillBeastSwimSearch;
extern AISTATE gillBeastSwimBite;
extern AISTATE gillBeastSwimRecoil;
extern AISTATE gillBeast13A138;
extern AISTATE gillBeast13A154;
extern AISTATE gillBeast13A170;

extern AISTATE handIdle;
extern AISTATE hand13A3B4;
extern AISTATE handSearch;
extern AISTATE handChase;
extern AISTATE handRecoil;
extern AISTATE handGoto;
extern AISTATE handJump;

extern AISTATE houndIdle;
extern AISTATE houndSearch;
extern AISTATE houndChase;
extern AISTATE houndRecoil;
extern AISTATE houndTeslaRecoil;
extern AISTATE houndGoto;
extern AISTATE houndBite;
extern AISTATE houndBurn;

extern AISTATE innocentIdle;
extern AISTATE innocentSearch;
extern AISTATE innocentChase;
extern AISTATE innocentRecoil;
extern AISTATE innocentTeslaRecoil;
extern AISTATE innocentGoto;

extern AISTATE podIdle;
extern AISTATE podMove;
extern AISTATE podSearch;
extern AISTATE podStartChase;
extern AISTATE podRecoil;
extern AISTATE podChase;
extern AISTATE tentacleIdle;
extern AISTATE tentacle13A6A8;
extern AISTATE tentacle13A6C4;
extern AISTATE tentacle13A6E0;
extern AISTATE tentacle13A6FC;
extern AISTATE tentacleMove;
extern AISTATE tentacleSearch;
extern AISTATE tentacleStartChase;
extern AISTATE tentacleRecoil;
extern AISTATE tentacleChase;

extern AISTATE ratIdle;
extern AISTATE ratSearch;
extern AISTATE ratChase;
extern AISTATE ratDodge;
extern AISTATE ratRecoil;
extern AISTATE ratGoto;
extern AISTATE ratBite;

extern AISTATE spidIdle;
extern AISTATE spidChase;
extern AISTATE spidDodge;
extern AISTATE spidGoto;
extern AISTATE spidSearch;
extern AISTATE spidBite;
extern AISTATE spidJump;
extern AISTATE spid13A92C;

extern AISTATE tchernobogIdle;
extern AISTATE tchernobogSearch;
extern AISTATE tchernobogChase;
extern AISTATE tchernobogRecoil;
extern AISTATE tcherno13A9B8;
extern AISTATE tcherno13A9D4;
extern AISTATE tcherno13A9F0;
extern AISTATE tcherno13AA0C;
extern AISTATE tcherno13AA28;

extern AISTATE zombieAIdle;
extern AISTATE zombieAChase;
extern AISTATE zombieAPonder;
extern AISTATE zombieAGoto;
extern AISTATE zombieAHack;
extern AISTATE zombieASearch;
extern AISTATE zombieARecoil;
extern AISTATE zombieATeslaRecoil;
extern AISTATE zombieARecoil2;
extern AISTATE zombieAStand;
extern AISTATE zombieEIdle;
extern AISTATE zombieEUp2;
extern AISTATE zombieEUp;
extern AISTATE zombie2Idle;
extern AISTATE zombie2Search;
extern AISTATE zombieSIdle;
extern AISTATE zombie13AC2C;

extern AISTATE zombieFIdle;
extern AISTATE zombieFChase;
extern AISTATE zombieFGoto;
extern AISTATE zombieFDodge;
extern AISTATE zombieFHack;
extern AISTATE zombieFPuke;
extern AISTATE zombieFThrow;
extern AISTATE zombieFSearch;
extern AISTATE zombieFRecoil;
extern AISTATE zombieFTeslaRecoil;


END_BLD_NS
