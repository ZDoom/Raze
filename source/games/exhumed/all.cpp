#include "src/2d.cpp"
#include "src/anims.cpp"
#include "src/anubis.cpp"
#include "src/bubbles.cpp"
#include "src/bullet.cpp"
#include "src/cd.cpp"
#include "src/cheats.cpp"
#include "src/enginesubs.cpp"
#include "src/exhumed.cpp"
#include "src/fish.cpp"
#include "src/gameloop.cpp"
#include "src/grenade.cpp"
#include "src/gun.cpp"
#include "src/init.cpp"
#include "src/input.cpp"
#include "src/items.cpp"
#include "src/lavadude.cpp"
#include "src/light.cpp"
#include "src/lighting.cpp"
#include "src/lion.cpp"
#include "src/map.cpp"
#include "src/menu.cpp"
#include "src/move.cpp"
#include "src/movie.cpp"
#include "src/mummy.cpp"
#include "src/object.cpp"
#include "src/osdcmds.cpp"
#include "src/player.cpp"
#include "src/queen.cpp"
#include "src/ra.cpp"
#include "src/ramses.cpp"
#include "src/random.cpp"
#include "src/rat.cpp"
#include "src/rex.cpp"
#include "src/roach.cpp"
#include "src/runlist.cpp"
#include "src/save.cpp"
#include "src/scorp.cpp"
#include "src/sequence.cpp"
#include "src/set.cpp"
#include "src/snake.cpp"
#include "src/sound.cpp"
#include "src/spider.cpp"
#include "src/status.cpp"
#include "src/switch.cpp"
#include "src/trigdat.cpp"
#include "src/view.cpp"
#include "src/wasp.cpp"
// This includes the VM so it is last
#include "src/d_menu.cpp"

// just to keep things temporarily clean.
BEGIN_PS_NS

void  FuncAnim(int nObject, int nMessage, int nDamage, int nRun)
{
    AIAnim ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void FuncAnubis(int nObject, int nMessage, int nDamage, int nRun)
{
    AIAnubis ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void  FuncBubble(int nObject, int nMessage, int nDamage, int nRun)
{
    AIBubble ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void FuncBullet(int nObject, int nMessage, int nDamage, int nRun)
{
    AIBullet ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void  FuncFishLimb(int nObject, int nMessage, int nDamage, int nRun)
{
    AIFishLimb ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}


void  FuncFish(int nObject, int nMessage, int nDamage, int nRun)
{
    AIFish ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void FuncGrenade(int nObject, int nMessage, int nDamage, int nRun)
{
    AIGrenade ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void  FuncLavaLimb(int nObject, int nMessage, int nDamage, int nRun)
{
    AILavaDudeLimb ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void  FuncLava(int nObject, int nMessage, int nDamage, int nRun)
{
    AILavaDude ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void FuncLion(int nObject, int nMessage, int nDamage, int nRun)
{
    AILion ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void  FuncCreatureChunk(int nObject, int nMessage, int nDamage, int nRun)
{
    AICreatureChunk ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);

}

void FuncMummy(int nObject, int nMessage, int nDamage, int nRun)
{
    AIMummy ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void FuncElev(int nObject, int nMessage, int nDamage, int nRun)
{
    AIElev ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void FuncWallFace(int nObject, int nMessage, int nDamage, int nRun)
{
    AIWallFace ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void FuncSlide(int nObject, int nMessage, int nDamage, int nRun)
{
    AISlide ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void FuncTrap(int nObject, int nMessage, int nDamage, int nRun)
{
    AITrap ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void FuncSpark(int nObject, int nMessage, int nDamage, int nRun)
{
    AISpark ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}


void FuncEnergyBlock(int nObject, int nMessage, int nDamage, int nRun)
{
    AIEnergyBlock ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}


void FuncObject(int nObject, int nMessage, int nDamage, int nRun)
{
    AIObject ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}


void FuncPlayer(int nObject, int nMessage, int nDamage, int nRun)
{
    AIPlayer ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}



void FuncQueenEgg(int nObject, int nMessage, int nDamage, int nRun)
{
    AIQueenEgg ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}


void FuncQueenHead(int nObject, int nMessage, int nDamage, int nRun)
{
    AIQueenHead ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void FuncQueen(int nObject, int nMessage, int nDamage, int nRun)
{
    AIQueen ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void FuncRa(int nObject, int nMessage, int nDamage, int nRun)
{
    AIRa ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);

}


void FuncRat(int nObject, int nMessage, int nDamage, int nRun)
{
    AIRat ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void FuncRex(int nObject, int nMessage, int nDamage, int nRun)
{
    AIRex ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void FuncRoach(int nObject, int nMessage, int nDamage, int nRun)
{
    AIRoach ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void FuncScorp(int nObject, int nMessage, int nDamage, int nRun)
{
    AIScorp ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}


void FuncSoul(int nObject, int nMessage, int nDamage, int nRun)
{
    AISoul ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void FuncSet(int nObject, int nMessage, int nDamage, int nRun)
{
    AISet ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}


void FuncSnake(int nObject, int nMessage, int nDamage, int nRun)
{
    AISnake ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void FuncSpider(int nObject, int nMessage, int nDamage, int nRun)
{
    AISpider ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

void FuncSwReady(int nObject, int nMessage, int, int nRun)
{
    AISWReady ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, 0, nRun);
}

void FuncSwPause(int nObject, int nMessage, int, int nRun)
{
    AISWPause ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, 0, nRun);
}

void FuncSwStepOn(int nObject, int nMessage, int, int nRun)
{
    AISWStepOn ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, 0, nRun);
}

void FuncSwNotOnPause(int nObject, int nMessage, int, int nRun)
{
    AISWNotOnPause ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, 0, nRun);
}

void FuncSwPressSector(int nObject, int nMessage, int, int nRun)
{
    AISWPressSector ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, 0, nRun);
}

void FuncSwPressWall(int nObject, int nMessage, int, int nRun)
{
    AISWPressWall ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, 0, nRun);
}

void FuncWasp(int nObject, int nMessage, int nDamage, int nRun)
{
    AIWasp ai;
    runlist_DispatchEvent(&ai, nObject, nMessage, nDamage, nRun);
}

AiFunc aiFunctions[kFuncMax] = {
    FuncElev,
    FuncSwReady,
    FuncSwPause,
    FuncSwStepOn,
    FuncSwNotOnPause,
    FuncSwPressSector,
    FuncSwPressWall,
    FuncWallFace,
    FuncSlide,
    FuncAnubis,
    FuncPlayer,
    FuncBullet,
    FuncSpider,
    FuncCreatureChunk,
    FuncMummy,
    FuncGrenade,
    FuncAnim,
    FuncSnake,
    FuncFish,
    FuncLion,
    FuncBubble,
    FuncLava,
    FuncLavaLimb,
    FuncObject,
    FuncRex,
    FuncSet,
    FuncQueen,
    FuncQueenHead,
    FuncRoach,
    FuncQueenEgg,
    FuncWasp,
    FuncTrap,
    FuncFishLimb,
    FuncRa,
    FuncScorp,
    FuncSoul,
    FuncRat,
    FuncEnergyBlock,
    FuncSpark,
};

// This is only temporary so that the event system can be refactored in smaller steps.
void runlist_DispatchEvent(ExhumedAI* ai, int nObject, int nMessage, int nDamage, int nRun)
{
    RunListEvent ev{};
    ev.nMessage = (EMessageType)(nMessage >> 16);
    ev.nObjIndex = RunData[nRun].nObjIndex;
    ev.pObjActor = RunData[nRun].pObjActor;
    ev.nParam = nObject;
    ev.nDamage = nDamage;
    ev.nRun = nRun;
    switch (ev.nMessage)
    {
    case EMessageType::ProcessChannel:
        ai->ProcessChannel(&ev);
        break;

    case EMessageType::Tick:
        ai->Tick(&ev);
        break;

    case EMessageType::Process:
        ai->Process(&ev);
        break;

    case EMessageType::Use:
        ai->Use(&ev);
        break;

    case EMessageType::TouchFloor:
        ai->TouchFloor(&ev);
        break;

    case EMessageType::LeaveSector:
        ai->LeaveSector(&ev);
        break;

    case EMessageType::EnterSector:
        ai->EnterSector(&ev);
        break;

    case EMessageType::Damage:
        ev.pOtherActor = &exhumedActors[nObject];
        ai->Damage(&ev);
        break;

    case EMessageType::Draw:
        ev.pTSprite = &mytsprite[nObject];
        ai->Draw(&ev);
        break;

    case EMessageType::RadialDamage:
        ev.nRadialDamage = nRadialDamage;
        ev.nDamageRadius = nDamageRadius;
        ev.pOtherActor = nullptr; // &exhumedActors[nObject]; nObject is always 0 here, this was setting some random invalid target
        ev.pRadialActor = pRadialActor;
        ai->RadialDamage(&ev);
        break;
    }
}


END_PS_NS
