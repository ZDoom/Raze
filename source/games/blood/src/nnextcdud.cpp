//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT
Copyright (C) NoOne

*********************************************************************
NoOne: Custom Dude system. Includes compatibility with older versions
For full documentation visit: http://cruo.bloodgame.ru/xxsystem/cdud/v2/
*********************************************************************

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

#ifdef NOONE_EXTENSIONS
#include "aicdud.h"
#include "nnextcdud.h"
#include "globals.h"
#include "gib.h"
#include "endgame.h"
#include "sound.h"
#include "view.h"
#include "ns.h"

BEGIN_BLD_NS


#define kParamMax 255
struct SEQCOMPAT
{
    uint8_t offset[3];
    int8_t nAiStateType;
};

struct SNDCOMPAT
{
    uint8_t offset;
    uint8_t range;
    uint8_t nSoundType;
};

struct WEAPINFO
{
    unsigned int type;
    const char* keyword;
    unsigned short range[2];
    unsigned int clipMask;
};

static const CUSTOMDUDE_SOUND_TPL gSoundTemplate[] =
{
    { { 1003, 1004, 1003 },  0x03, true,   true,  true,  0 },  // spot
    { { 1013, 1014, 1013 },  0x03, true,   true,  true,  0 },  // pain
    { { 1018, 1019, 1018 },  0x03, true,   true,  true,  0 },  // death
    { { 1031, 1032, 1031 },  0x03, true,   true,  true,  0 },  // burning state
    { { 1018, 1019, 1018 },  0x03, true,   true,  true,  0 },  // explosive death or end of burning state
    { { 4021, 4022, 4021 },  0x03, true,   true,  true,  0 },  // target of dude is dead
    { { 1005, 1006, 1005 },  0x03, true,   true,  true,  0 },  // target chase
    { { 9008, 9008, 9008 },  0x03, true,   true,  true,  0 },  // morph in another dude
    { { 745,  745,  745 },   0x03, true,   true,  true,  0 },  // wake after sleeping
};

// v1 dude animation compatibility
static const SEQCOMPAT gSeqCompat[] =
{
    { { 0,  17, 13 },  kCdudeStateIdle,             },
    { { 0,  17, 13 },  kCdudeStateGenIdle,          },
    { { 9,  14, 13 },  kCdudeStateSearch,           },
    { { 9,  14, 13 },  kCdudeStateDodge,            },
    { { 9,  14, 13 },  kCdudeStateChase,            },
    { { 9,  14, 13 },  kCdudeStateFlee,             },
    { { 5,   5,  5 },  kCdudeStateRecoil,           },
    { { 4,   4,  4 },  kCdudeStateRecoilT,          },
    { { 6,   8,  8 },  kCdudeStateAttackBase,       },
    { { 7,   7,  7 },  kCdudeStateAttackBase + 1,   },
    { { 10, 10, 10 },  kCdudeStateAttackBase + 2,   },
    { { 3,   3,  3 },  kCdudeBurnStateSearch,       },
    { { 18, 18, 18 },  kCdudeStateMorph,            },
    { { 1,   1,  1 },  kCdudeStateDeathBase,        },
    { { 2,   2,  2 },  kCdudeStateDeathExplode,     },
    { { 15, 15, 15 },  kCdudeStateDeathBurn,        },
};

// v1 dude sound compatibility
static const SNDCOMPAT gSndCompat[] =
{
    {0,     2,     kCdudeSndTargetSpot},      {2,     2,     kCdudeSndGotHit},
    {4,     2,     kCdudeSndDeathNormal},     {6,     2,     kCdudeSndBurning},
    {8,     2,     kCdudeSndDeathExplode},    {10,    2,     kCdudeSndTargetDead},
    {12,    2,     kCdudeSndTargetChase},     {14,    1,     kCdudeSndCompatAttack1},
    {15,    1,     kCdudeSndCompatAttack2},   {16,    1,     kCdudeSndCompatAttack3},
    {17,    1,     kCdudeSndTransforming},
};


/*************************************************************************************************/
static constexpr const char* gErrors[kErrMax] =
{
    "The value \"%s\" is not correct %s value",
    "Invalid parameter \"%s\"",
    "Invalid position of parameter \"%s\"",
    "Correct range of \"%s\" is %d - %d",
    "\"%s\" is not valid value of \"%s\"",
    "Invalid descriptor version",
    "Required parameter \"%s\" not found",
    "Invalid array length! Expected: %d. Given: %d",
    "Invalid array length! Expected: %d - %d. Given: %d",
    "Required group \"%s\" not found",
    "Invalid position of value \"%s\"",
};

static constexpr const char* gValTypes[kValMax] =
{
    "NONE",
    "ANY",
    "FIXED",
    "UNSIGNED FIXED",
    "PERCENT",
    "COMMON ARRAY",
    "ASSOCIATIVE ARRAY",
    "BOOLEAN",
    "CDUD",
    "WEAPON",
    "FX",
    "GIB",
    "VECTOR",
    "PROJECTILE",
    "THROW",
    "VDUD",
    "KAMIKAZE",
    "SPECIAL",
};

static const WEAPINFO gCdudeWeaponInfo[] =
{
    { kCdudeWeaponNone,              gValTypes[kValNone],        { 0, 0 },                                                   0 },
    { kCdudeWeaponHitscan,           gValTypes[kValVector],      { 0, kVectorMax },                                         CLIPMASK1 },
    { kCdudeWeaponMissile,           gValTypes[kValProjectile],  { kMissileBase, kMissileMax },                             CLIPMASK0 },
    { kCdudeWeaponThrow,             gValTypes[kValThrow],       { kThingBase, kThingMax},                                  CLIPMASK0 },
    { kCdudeWeaponSummon,            gValTypes[kValVdud],        { kDudeCultistTommy, kDudeVanillaMax },                    CLIPMASK0 },
    { kCdudeWeaponSummonCdude,       gValTypes[kValCdud],        { 1, 10000 },                                              CLIPMASK0 },
    { kCdudeWeaponKamikaze,          gValTypes[kValKamikaze],    { kTrapExploder, kTrapExploder + kExplosionMax },          0 },
    { kCdudeWeaponSpecialBeastStomp, gValTypes[kValSpecial],     { kCdudeWeaponIdSpecialBase, kCdudeWeaponIdSpecialMax },   0},
};

/*************************************************************************************************/
static const PARAM gParGroup[] =
{
    {kParGroupGeneral,              "General"   },              {kParGroupVelocity,     "Velocity"  },
    {kParGroupAnimation,            "Animation" },              {kParGroupSound,        "Sound"     },
    {kParGroupWeapon,               "Weapon%d"  },              {kParGroupDodge,        "Dodge"     },
    {kParGroupRecoil,               "Recoil"    },              {kParGroupDamage,       "Damage"    },
    {kParGroupKnockout,             "Knockout"  },              {kParGroupFXEffect,     "FXEffect%d"},
    {kParGroupDropItem,             "DropItem"  },              {kParGroupMovePat,      "MoveSetup" },
    {kParGroupParser,               "Parser"    },
    {kParamMax, nullptr},
};

static const PARAM gParamGeneral[] =
{
    {kParGeneralVersion,            "Version" },                 {kParGeneralMass,           "Mass"            },
    {kParGeneralMedium,             "Medium"  },                 {kParGeneralClipdist,       "Clipdist"        },
    {kParGeneralHealth,             "Health"  },                 {kParGeneralMorphTo,        "MorphTo"         },
    {kParGeneralActiveTime,         "SearchTime"},
    {kParamMax, nullptr},
};

static const PARAM gParamParser[] =
{
    {kParParserWarnings,       "ShowWarnings"},
    {kParamMax, nullptr},
};

static const PARAM gParamAnim[] =
{
    {kCdudeStateIdle,           "Idle"          },      {kCdudeStateSearch,         "Search"        },
    {kCdudeStateDodge,          "Dodge"         },      {kCdudeStateChase,          "Chase"         },
    {kCdudeStateFlee,           "Flee"          },      {kCdudeStateRecoil,         "RecoilDefault" },
    {kCdudeStateRecoilT,        "RecoilElectric"},      {kCdudeBurnStateSearch,     "Burning"       },
    {kCdudeStateMorph,          "Morphing"      },      {kCdudeStateKnockEnter,     "KnockoutEnter" },
    {kCdudeStateKnock,          "Knockout"      },      {kCdudeStateKnockExit,      "KnockoutExit"  },
    {kCdudeStateSleep,          "IdleSleep"     },      {kCdudeStateWake,           "IdleWake"      },
    {kCdudeStateDeathBase,      "DeathDefault"  },      {kCdudeStateDeathBurn,      "DeathBurning"  },
    {kCdudeStateDeathVector,    "DeathBullet"   },      {kCdudeStateDeathExplode,   "DeathExplode"  },
    {kCdudeStateDeathChoke,     "DeathChoke"    },      {kCdudeStateDeathSpirit,    "DeathSpirit"   },
    {kCdudeStateDeathElectric,  "DeathElectric" },      {kCdudeAnimScale,           "Scale"         },
    {kCdudeStateDeath,          "Death"         },      {kCdudeStateMove,           "Move"          },
    {kCdudeStateAttack,         "Attack"        },
    {kParamMax, nullptr},
};

static const PARAM gParamSounds[] =
{
    {kCdudeSndTargetSpot,           "TargetSpot"  },          {kCdudeSndTargetChase,      "TargetChase"       },
    {kCdudeSndTargetDead,           "TargetDead"  },          {kCdudeSndGotHit,           "GotHit"            },
    {kCdudeSndBurning,              "Burning"     },          {kCdudeSndDeathNormal,      "DeathDefault"      },
    {kCdudeSndDeathExplode,         "DeathBurning"},          {kCdudeSndTransforming,     "Morphing"          },
    {kCdudeSndWake,                 "IdleWake"    },
    {kParamMax, nullptr},
};

static const PARAM gParamWeapon[] =
{
    {kParWeaponId,                  "Type"       },          {kParWeaponDist,                "Distance"        },
    {kParWeaponDisp,                "Dispersion" },          {kParWeaponAttackAng,           "AimAngle"        },
    {kParWeaponMedium,              "Medium"     },          {kParWeaponAmmo,                "Ammo"            },
    {kParWeaponPickChance,          "PickChance" },          {kParWeaponShotAppearance,      "ShotAppearance"  },
    {kParWeaponShotSnd,             "ShotSound"  },          {kParWeaponAttackAnim,          "AttackAnimation" },
    {kParWeaponShotSetup,           "ShotSetup"  },          {kParWeaponAttackSetup,         "AttackSetup"     },
    {kParWeaponPosture,             "Posture"    },          {kParWeaponTargetHealth,        "TargetHealth"    },
    {kParWeaponDudeHealth,          "DudeHealth" },          {kParWeaponAkimbo,              "Akimbo"          },
    {kParWeaponSkill,               "Skill"      },          {kParWeaponCooldown,            "Cooldown"        },
    {kParWeaponStyle,               "Style"      },
    {kParamMax, nullptr},
};

static const PARAM gParamWeaponStyle[] =
{
    {kParWeaponStyleOffset,         "Offset"        },
    {kParWeaponStyleAngle,          "Angle"         },
    {kParamMax, nullptr},
};

static const PARAM gParamAttack[] =
{
    {kParAttackTime,                "Timer"         },
    {kParAttackInterrupt,           "Interruptable" },
    {kParAttackTurn2Target,         "TurnToTarget"  },
    {kParAttackNumShots,            "NumShots"      },
    {kParAttackInertia,             "Inertia"       },
    {kParamMax, nullptr},
};

static const PARAM gWeaponShotSetup[] =
{
    {kParWeaponShotOffs,            "Offset"        },          {kParWeaponShotVel,         "Velocity"          },
    {kParWeaponShotSlope,           "Slope"         },          {kParWeaponShotFollow,      "FollowAngle"       },
    {kParWeaponShotClipdist,        "Clipdist"      },          {kParWeaponShotImpact,      "Impact"            },
    {kParWeaponShotRemTimer,        "LiveTime"      },
    {kParamMax, nullptr},
};

static const PARAM gParamOnEvent[] =
{
    {kParEventOnDmg,                "OnDamage"      },
    {kParEventOnAimTargetWrong,     "OnAimMiss"     },
    {kParamMax, nullptr},
};
static const PARAM* gParamDodge = gParamOnEvent;
static const PARAM* gParamRecoil = gParamOnEvent;
static const PARAM* gParamKnockout = gParamOnEvent;

static const PARAM gParamVelocity[] =
{
    {kParVelocityForward,   "Forward" },
    {kParVelocityTurn,      "Turn"    },
    {kParVelocityDodge,     "Dodge"   },
    {kParamMax, nullptr},
};

static const PARAM gParamPosture[] =
{
    {kCdudePostureL,        "Stand"  },
    {kCdudePostureC,        "Crouch" },
    {kCdudePostureW,        "Swim"   },
    {kParamMax, nullptr},
};


static const PARAM gParamMedium[]
{
    {kParMediumAny,         "Any"   },
    {kParMediumLand,        "Land"  },
    {kParMediumWater,       "Water" },
    {kParamMax, nullptr},
};

static const PARAM gParamDamage[] =
{
    {kDmgFall,              "Falling"          },              {kDmgBurn,              "Burning"          },
    {kDmgBullet,            "Bullet"           },              {kDmgExplode,           "Explode"          },
    {kDmgChoke,             "Choke"            },              {kDmgSpirit,            "Spirit"           },
    {kDmgElectric,          "Electric"         },              {kParDmgSource,         "IgnoreDamageFrom" },
    {kParamMax, nullptr},
};

static const PARAM gParamDamageSource[] =
{
    {kDmgSourcePlayer,      "Player" },                      {kDmgSourceDude,        "Dude" },
    {kDmgSourceSelf,        "Self"   },                      {kDmgSourceFriend,      "Friend" },
    {kDmgSourceEnemy,       "Enemy"  },                      {kDmgSourceSlave,       "Slave" },
    {kDmgSourceKin,         "Kin"    },
    {kParamMax, nullptr},
};

static const PARAM gParamAppearance[] =
{
    {kAppearSeq,            "Seq"   },                       {kAppearClb,        "Callback" },
    {kAppearSnd,            "Sound" },                       {kAppearScale,      "Scale"    },
    {kAppearPic,            "Tile"  },                       {kAppearShade,      "Shade"    },
    {kAppearPal,            "Pal"   },                       {kAppearSize,       "Size"     },
    {kAppearOffs1,          "Offset"},
    {kParamMax, nullptr},
};

static const PARAM gParamEffect[] =
{
    {kParEffectId,          "Type"          },     {kParEffectTimer,        "Delay"             },
    {kParEffectOffset,      "Offset"        },     {kParEffectAppearance,   "Appearance"        },
    {kParEffectAiState,     "OnState"       },     {kParEffectAnimFrame,    "AnimFrame"         },
    {kParEffectVelocity,    "Velocity"      },     {kParEffectAngle,        "Angle"             },
    {kParEffectSlope,       "VelocitySlope" },     {kParEffectPosture,      "Posture"           },
    {kParEffectMedium,      "Medium"        },     {kParEffectRemTime,      "LiveTime"          },
    {kParEffectChance,      "SpawnChance"   },     {kParEffectAllUnique,    "SpawnAll"          },
    {kParEffectAnimID,      "OnAnim"        },     {kParEffectSrcVel,       "AddSourceVelocity" },
    {kParEffectFx2Gib,      "GibSetup"      },
    {kParamMax, nullptr},
};

static const PARAM gParamMorphTo[] =
{
    {kValVdud, gValTypes[kValVdud]},
    {kValCdud, gValTypes[kValCdud]},
    {kParamMax, nullptr},
};

static const PARAM gParamMovePat[] =
{
    {kParMoveFallHeight,    "FallHeight"},
    {kParamMax, nullptr},
};

static const PARAM gParamDropItem[] =
{
    {kParDropItem,              "Item%d"},                  {kParDropItemType,          "Type"},
    {kParDropItemChance,        "Chance"},                  {kParDropItemSkill,         "Skill"},
    {kParDropItemSprChance,     "SpriteDropItemChance"},
    {kParamMax, nullptr},
};

static const PARAM gParamGib[] =
{
    {kParGibType,               "Type"},        {kParGibForce,              "Force"},
    {kParGibSoundID,            "SoundID"},     {kParGibFlags,              "Flags"},
    {kParGibPhysics,            "Physics"},
    {kParamMax, nullptr},
};

static const PARAM gParamTriggerFlags[] =
{
    {kParTigNone,               "None"  },         {kParTrigVector,            "Vector"},
    {kParTrigImpact,            "Impact"},         {kParTrigTouch,             "Touch" },
    {kParTrigLocked,            "Locked"},
    {kParamMax, nullptr},
};

static const PARAM gParamPhysics[] =
{
    {0,                             "None"},
    {kPhysGravity | kPhysFalling,   "Gravity"},
    {kPhysMove,                     "Motion"},
    {kParamMax, nullptr},
};


VMNativeFunction** const gCdudeCustomCallback[] =
{
    &AF(fxFlameLick),           // 1
    &AF(fxFlareSpark),          // 2
    &AF(fxFlareSparkLite),      // 3
    &AF(fxBloodSpurt),          // 4 replace kCallbackFXZombieSpurt
    &AF(fxBloodSpurt),          // 5
    &AF(fxArcSpark),            // 6
    &AF(fxDynPuff),             // 7
    &AF(fxTeslaAlt),            // 8
    &AF(fxPodGreenBloodSpray),  // 9
    &AF(fxPodBloodSpray),       // 10
};

static bool CheckProximity(DBloodActor* pSpr1, DBloodActor* pSpr2, int nDist) = delete; // make this error out for refactoring

static bool CheckProximity(DBloodActor* pSpr1, DBloodActor* pSpr2, double nDist)
{
    return CheckProximityPoint(pSpr1->spr.pos.X, pSpr1->spr.pos.Y, pSpr1->spr.pos.Z, pSpr2->spr.pos.X, pSpr2->spr.pos.Y, pSpr2->spr.pos.Z, nDist);
}

static bool CanSee(DBloodActor* pSpr1, DBloodActor* pSpr2)
{
    return cansee(pSpr1->spr.pos, pSpr1->sector(), pSpr2->spr.pos, pSpr2->sector());
}

static void GetExtents(DBloodActor* pSprite, double* top, double* bottom, FTextureID nPic);
static int AreaOfSector(sectortype* pSector);
static bool isIdKeyword(const char* fullStr, const char* prefix, int* nID = nullptr);
static bool isNone(const char* str) { return (stricmp(str, gValTypes[kValNone]) == 0); }
static int qsSortWeapons(CUSTOMDUDE_WEAPON* ref1, CUSTOMDUDE_WEAPON* ref2) { return ref1->pickChance - ref2->pickChance; }
static bool helperSeqExists(int nSeq);
static FTextureID helperGetFirstPic(DCustomDude* pDude);
static bool helperSeqTriggerExists(int nSeq);
static Seq* helperSeqLoad(int nSeq);
static Seq* helperSeqLock(int nSeq);


/*
DCustomDude* CUSTOMDUDE_SETUP::pDude;
IniFile* CUSTOMDUDE_SETUP::pIni;
DICTNODE* CUSTOMDUDE_SETUP::hIni;

bool CUSTOMDUDE_SETUP::key[256];
bool CUSTOMDUDE_SETUP::val[256];
bool CUSTOMDUDE_SETUP::showWarnings;
PARAM* CUSTOMDUDE_SETUP::pGroup;
PARAM* CUSTOMDUDE_SETUP::pParam;
const char* CUSTOMDUDE_SETUP::pValue;
int CUSTOMDUDE_SETUP::nWarnings;
*/

CUSTOMDUDE_SETUP setup;


/*************************************************************************************************/


bool DCustomDude::IsMediumMatch(int nMedium)
{
    if (!nMedium)
        return true;
    
    bool uwater = IsUnderwater();
    return (((nMedium & 0x01) && !uwater) || ((nMedium & 0x02) && uwater));
}

bool DCustomDude::IsPostureMatch(int nPosture)
{
    if (!nPosture) return true;
    if ((nPosture & 0x01) && posture == kCdudePostureL)  return true;
    if ((nPosture & 0x02) && posture == kCdudePostureC)  return true;
    if ((nPosture & 0x04) && posture == kCdudePostureW)  return true;
    return false;
}

double DCustomDude::AdjustSlope(DBloodActor* pTarget, double zOffs)
{
    if (pTarget)
    {
        DVector3 dv = pTarget->spr.pos - pSpr->spr.pos;
        return (dv.Z - zOffs) / dv.XY().Length(); // todo: verify the factor, should be 1, was divscale10
    }
    return 0;
}

bool DCustomDude::AdjustSlope(double nDist, double* nSlope)
{
    DBloodActor* pTarget = pSpr->xspr.target;
    if (pTarget && pWeapon)
    {
        double nStep, zTop, zBot;
        GetActorExtents(pTarget, &zTop, &zBot);

        nStep = max(abs(zTop - zBot), 10 * inttoworld);
        while (nDist > 0)
        {
            *nSlope = (pTarget->spr.pos.Z - pSpr->spr.pos.Z) / nDist; // todo: verify the factor, should be 1, was divscale10
            HitScan(pSpr, pSpr->spr.pos.Z, DVector3(pSpr->spr.Angles.Yaw.ToVector(), *nSlope), pWeapon->clipMask, nDist);
            if (pSpr->xspr.target == gHitInfo.actor())
                return true;

            nDist -= nStep;
        }
    }

    return false;
}

CUSTOMDUDE_WEAPON* DCustomDude::PickWeapon(ARG_PICK_WEAPON* pArg)
{
    CUSTOMDUDE_WEAPON* pRetn = nullptr;
    CUSTOMDUDE_WEAPON* pWeap;
    int i;

    for (i = 0; i < numWeapons; i++)
    {
        pWeap = &weapons[i];
        
        pWeap->available = 0;
        if (!pWeap->IsTimeout())
        {
            if (pWeap->cooldown.totalUseCount)
            {
                if (pWeap->cooldown.useCount >= pWeap->cooldown.totalUseCount)
                    pWeap->cooldown.useCount = 0;
            }

            if (pArg->angle < pWeap->angle && rngok(pArg->_distance, pWeap->distRange[0], pWeap->distRange[1]))
            {
                if (pWeap->HaveAmmmo() && IsMediumMatch(pWeap->medium) && IsPostureMatch(pWeap->posture))
                {
                    if (irngok(pArg->dudeHealth, pWeap->dudeHpRange[0], pWeap->dudeHpRange[1]))
                    {
                        if (irngok(pArg->targHealth, pWeap->targHpRange[0], pWeap->targHpRange[1]))
                        {
                            pWeap->available = 1;
                            if (!pRetn && Chance(pWeap->pickChance))
                                pRetn = pWeap;
                        }
                    }
                }
            }
        }
    }

    return pRetn;
}

bool DCustomDude::IsTooTight(void)
{
    if (pSpr->sector())
    {
        double fZ, cZ, zTop, zBot;
        calcSlope(pSpr->sector(), pSpr->spr.pos, &cZ, &fZ);
        GetExtents(pSpr, &zTop, &zBot, largestPic);
        return (zBot >= fZ && zTop < cZ - 2);
    }

    return false;
}

int DCustomDude::GetDamage(DBloodActor* pSrc, int nDmgType)
{
    int nDamage = damage.id[nDmgType];
    int nIgnore  = damage.ignoreSources;

    if (IsSleeping())
        nDamage <<= 4;

    if (pSrc == nullptr || !nIgnore)
        return nDamage;

    bool play           = pSrc->IsPlayerActor();
    bool dude           = pSrc->IsDudeActor();
    bool self           = (pSrc == pSpr);

    if (dude && !play && !self)
    {
        if (nIgnore & kDmgSourceKin)
        {
            if (pSrc->hasX() && pSrc->GetType() == pSpr->GetType() && pSrc->xspr.data1 == pSpr->xspr.data1)
                return 0;
        }

        if (nIgnore & kDmgSourceSlave)
        {
            if (pSlaves.Contains(MakeObjPtr(pSrc)))
                return 0;
        }

        if (nIgnore & kDmgSourceEnemy)
        {
            if (pSpr->xspr.target == pSrc || (pSrc->hasX() && pSrc->xspr.target == pSpr))
                return 0;
        }

        if (nIgnore & kDmgSourceFriend)
        {
            if (pSrc->hasX() && pSrc->xspr.target == pSpr->xspr.target)
                return 0;
        }
    }

    if ((nIgnore & kDmgSourcePlayer) && play)                          return 0;
    if ((nIgnore & kDmgSourceDude) && dude && !play && !self)          return 0;
    if ((nIgnore & kDmgSourceSelf) && self)                            return 0;
    return nDamage;
}

void DCustomDude::Process(void)
{
    int nOldPosture = posture;
    
    if (pSpr->xspr.aiState->moveFunc)
        callActorFunction(*pSpr->xspr.aiState->moveFunc, pSpr);

    if (pSpr->xspr.aiState->thinkFunc && (gFrameCount & 3) == (pSpr->GetIndex() & 3))
        callActorFunction(*pSpr->xspr.aiState->thinkFunc, pSpr);

    if (!IsMorphing() && !IsDying())
    {
        if (!IsMediumMatch(medium))
        {
            Kill(pSpr, kDamageFall, pSpr->xspr.health << 4);
            return;
        }
        
        UpdateSlaves();

        if (IsUnderwater())
        {
            posture = kCdudePostureW;
            if (pSpr->xspr.burnTime)
            {
                evKillActor(pSpr, AF(fxFlameLick));
                pSpr->xspr.burnTime = 0;
            }
        }
        else if (!IsCrouching())
        {
            if (CanCrouch() && IsTooTight())
            {
                posture = kCdudePostureC;
            }
            else
            {
                posture = kCdudePostureL;
            }
        }
        else if (prevSector != pSpr->sector())
        {
            if (!IsTooTight() && AreaOfSector(pSpr->sector()) > 0x2000)
                posture = kCdudePostureL;

            prevSector = pSpr->sector();
        }

        if (nOldPosture != posture)
            SyncState(); // have to sync posture and animations...
    }

    if (pSpr->xspr.aiState->nextState && !pSpr->xspr.stateTimer)
    {
        if (pSpr->xspr.aiState->stateTicks > 0 || !SeqPlaying())
            NewState(pSpr->xspr.aiState->nextState);
    }

    if (numEffects)
        ProcessEffects();
}

void DCustomDude::ProcessEffects(void)
{
    CUSTOMDUDE_EFFECT* pEff;
    DBloodActor* pPlay;
    int nS, nP;
    int i, j;
    
    for (i = connecthead; i >= 0; i = connectpoint2[i])
    {
        pPlay = getPlayer(i)->GetActor();
        if (!CheckProximity(pPlay, pSpr, 128.) && !CanSee(pSpr, pPlay))
            continue;

        for (j = 0; j < numEffects; j++)
        {
            pEff = &effects[j];
            if ((unsigned int)gFrameCount < pEff->clock || !IsPostureMatch(pEff->posture) || !IsMediumMatch(pEff->medium))
                continue;

            if (pEff->pStates.Size())
            {
                if (aiInPatrolState(pSpr->xspr.aiState))
                {
                    if ((aiPatrolMoving(pSpr->xspr.aiState) || aiPatrolTurning(pSpr->xspr.aiState)) && pEff->pStates.Contains(kCdudeStateMove));
                    else if (aiPatrolWaiting(pSpr->xspr.aiState) && pEff->pStates.Contains(kCdudeStateIdle));
                    else continue;
                }
                else if (!FindState(pSpr->xspr.aiState, &nS, &nP) || !pEff->pStates.Contains(nS))
                {
                    continue;
                }
            }

            if (pEff->CanSpawn(pSpr))
            {
                pEff->Spawn(pSpr);
                pEff->SetDelay();
            }
        }

        break;
    }
}

bool DCustomDude::NewState(AISTATE* pState)
{
    if (!IsMorphing())
    {
        pSpr->xspr.stateTimer = pState->stateTicks;
        pSpr->xspr.aiState = pState;

        if (pState->seqId > 0)
        {
            seqSpawn(pState->seqId, pSpr, *pState->funcId);
        }
        else if (pState->seqId == 0)
        {
            seqKill(pSpr);
        }

        if (pState->enterFunc)
            callActorFunction(*pState->enterFunc, pSpr);

        return true;
    }

    return false;
}

void DCustomDude::NewState(int nStateType, int nTimeOverride)
{
    AISTATE* pTmp   = &states[nStateType][posture];
    AISTATE* pState = &states[nStateType][kCdudePostureL];
    if (pTmp->seqId > 0)
        pState = pTmp;

    if (NewState(pState))
    {
        if (nTimeOverride >= 0)
            pSpr->xspr.stateTimer = nTimeOverride;
    }
}

void DCustomDude::NextState(AISTATE* pState, int nTimeOverride)
{
    pSpr->xspr.aiState->nextState = pState;
    if (pSpr->xspr.aiState->nextState && nTimeOverride > 0)
        pSpr->xspr.aiState->nextState->stateTicks = nTimeOverride;
}

void DCustomDude::NextState(int nStateType, int nTimeOverride)
{
    NextState((nStateType >= 0) ? &states[nStateType][posture] : nullptr, nTimeOverride);
}

void DCustomDude::SyncState(void)
{
    int t1, t2;
    if (pSpr->xspr.aiState && FindState(pSpr->xspr.aiState, &t1, &t2) && t2 != posture)
    {
        t2 = pSpr->xspr.stateTimer; // save time
        AISTATE* pState = &states[t1][posture];
            
        NewState(pState);
        if (pState->stateTicks)
            pSpr->xspr.stateTimer = t2; // restore time
    }
}

bool DCustomDude::FindState(AISTATE* pState, int* nStateType, int* nPosture)
{
    return setup.FindAiState(states, countof(states), pState, nStateType, nPosture);
}

void DCustomDude::InitSprite(void)
{
    assert(initialized != 0);

    if (StatusTest(kCdudeStatusRespawn))
    {
        auto pSect = pSpr->sector(); // it does not change sector on respawn
        updatesector(pSpr->spr.pos.XY(), &pSect);
        if (pSect && pSect != pSpr->sector())
            ChangeActorSect(pSpr, pSect);

        StatusRem(kCdudeStatusAwaked);
        StatusRem(kCdudeStatusRespawn);
        StatusRem(kCdudeStatusBurning);
        StatusRem(kCdudeStatusDying);
    }

    if (IsBurning())
    {
        NewState(kCdudeBurnStateSearch);
        pSpr->xspr.burnTime = 1200;
    }
    else
    {
        pExtra->active    = 0;
        pExtra->thinkTime = 0;
        if (spriteIsUnderwater(pSpr))
        {
            pSpr->xspr.medium   = kMediumWater;
            posture         = kCdudePostureW;
        }
        else
        {
            pSpr->xspr.medium   = kMediumNormal;
            posture         = kCdudePostureL;
        }

        if (IsMorphing())       NewState(kCdudeStateMorph);
        else if (CanSleep())    NewState(kCdudeStateSleep);
        else
        {
            if (StatusTest(kCdudeStatusForceCrouch) && posture == kCdudePostureL)
            {
                if (GetStateSeq(kCdudeStateIdle, kCdudePostureC))
                {
                    posture     = kCdudePostureC;
                }
                else
                {
                    StatusRem(kCdudeStatusForceCrouch);
                }
            }

            NewState(kCdudeStateIdle);
        }
    }

    if (pSpr->xspr.scale)
        nnExtSprScaleSet(pSpr, pSpr->xspr.scale);

    pSpr->spr.cstat |= CSTAT_SPRITE_RESTORE | CSTAT_SPRITE_BLOCK_HITSCAN | CSTAT_SPRITE_BLOCK;
    pSpr->spr.flags = (kPhysMove | kPhysGravity | kPhysFalling);
    prevSector = pSpr->sector();
}

void DCustomDude::Activate(void)
{
    if (IsDying())
        return;
    
    if (!IsBurning())
    {
        if (!IsMorphing())
        {
            if (!IsSleeping())
            {
                if (!pExtra->active && StatusTest(kCdudeStatusForceCrouch) && IsCrouching())
                    posture = kCdudePostureL;

                pExtra->active = 1;
                NewState(kCdudeStateChase);
            }
            else
            {
                NewState(kCdudeStateWake);
                StatusSet(kCdudeStatusAwaked); // once awaked cannot enter in sleep anymore (unless respawn)
            }
        }
    }
    else
    {
        NewState(kCdudeBurnStateSearch);
    }
}

int DCustomDude::Damage(DBloodActor* nFrom, int nDmgType, int nDmg)
{
    int t = pSpr->cumulDamage;

    if (IsBurning())
    {
        if (IsUnderwater())
        {
            pSpr->ChangeType(kDudeModernCustom);
            pInfo = getDudeInfo(pSpr->GetType());
            pSpr->xspr.burnSource = -1;
            pSpr->xspr.burnTime = 0;
            pSpr->xspr.health = 1;

            NewState(kCdudeStateSearch);
        }
        else if (pSpr->xspr.burnTime <= 0)
        {
            PlaySound(kCdudeSndBurning);
            pSpr->xspr.burnTime = 1200;
        }
    }
    else if (nDmgType == kDamageBurn)
    {
        if (pSpr->xspr.health <= (uint32_t)pInfo->fleeHealth)
        {
            pSpr->xspr.health = 0; // let the actDamageSprite to call actKillDude
            return nDmg;
        }
    }
    else if (pSpr->xspr.health)
    {
        if (IsKnockout())
        {
            pExtra->teslaHit = 0;
            return nDmg;
        }
        else if (CanKnockout() && knockout.Allow(t))
        {
            NewState(kCdudeStateKnockEnter);
            PlaySound(kCdudeSndGotHit);
            pExtra->teslaHit = 0;
            return nDmg;
        }

        if (nDmgType == kDmgElectric)
            pExtra->teslaHit = 1;

        if (CanRecoil() && recoil.Allow(t))
            Recoil();

        if (CanMove() && dodge.onDamage.Allow(t))
        {
            t = (30 * Random(2));
            if (t == 0)
                t = 90;

            if (IsRecoil())
            {
                NextState(kCdudeStateDodge, t);
            }
            else
            {
                if (Chance(0x0500))
                    PlaySound(kCdudeSndGotHit);

                NewState(kCdudeStateDodge, t);
            }
        }
        else if (IsRecoil())
        {
            NextState(kCdudeStateChase);
        }
    }

    pExtra->teslaHit = 0;
    return nDmg;
}

void DCustomDude::Recoil(void)
{
    int nState = -1;
    if (!IsKnockout())
    {
        if (CanRecoil())
            nState = kCdudeStateRecoil;

        if (pExtra->teslaHit)
        {
            if (CanElectrocute() && !IsUnderwater())
            {
                nState = kCdudeStateRecoilT;
            }
        }

        PlaySound(kCdudeSndGotHit);
        NewState(nState);
    }

    pExtra->teslaHit = 0;
}

AISTATE* DCustomDude::PickDeath(int nDmgType)
{
    int i, nRand = Random(kCdudePostureMax);
    AISTATE* pDeath = &states[kCdudeStateDeathBase + nDmgType][nRand];
    if (pDeath->seqId > 0)
        return pDeath;

    // find first existing animation
    pDeath = states[kCdudeStateDeathBase + nDmgType];
    for (i = 0; i < kCdudePostureMax; i++)
    {
        if (pDeath[i].seqId > 0)
            return &pDeath[i];
    }

    // use default death
    return &states[kCdudeStateDeathBase][nRand];
}

void DCustomDude::Kill(DBloodActor* pFrom, int nDmgType, int nDmg)
{
    GAMEOPTIONS* pOpt = &gGameOptions;
    AISTATE* pDeath;
    int i;

    if (IsDying())
        return;

    if (!std::get_if<bool>(&nextDude))
    {
        // clamp hp so is not count as dead
        pSpr->xspr.health = ClipLow(pSpr->xspr.health, 16);
        if (!IsMorphing())
            NewState(kCdudeStateMorph);

        return;
    }

    if (pWeapon && pWeapon->type == kCdudeWeaponKamikaze)
    {
        if (!IsAttacking())
        {
            switch (nDmgType)
            {
                case kDamageSpirit:
                    break;
                default:
                    if (!Chance(0x2000)) break;
                    [[fallthrough]];
                case kDamageBurn:
                case kDamageExplode:
                case kDamageBullet:
                    pSpr->xspr.health = 0; // so it won't repeat killing
                    cdudeDoExplosion(this);
                    break;
            }
        }
    }
    else if (nDmgType == kDamageBurn)
    {
        if (!IsUnderwater())
        {
            if (CanBurn() && !IsBurning())
            {
                evKillActor(pSpr, AF(fxFlameLick));
                sfxPlay3DSound(pSpr, 361, -1, -1);
                StatusSet(kCdudeStatusBurning);
                PlaySound(kCdudeSndBurning);

                pSpr->xspr.health = kCdudeBurningHealth;
                NewState(kCdudeBurnStateSearch);
                damage.Set(256, kDmgBurn);
                return;
            }
        }
        else
        {
            pSpr->xspr.burnTime = 0;
            pSpr->xspr.burnSource = -1;
            nDmgType = kDamageFall;
        }
    }
    else if (IsBurning())
    {
        nDmgType = kDamageBurn;
    }

    ClearEffectCallbacks();
    LeechKill(true);
    DropItems();

    if (pOpt->nGameType != kGameTypeSinglePlayer)
    {
        for (i = connecthead; i >= 0; i = connectpoint2[i])
        {
            auto pPlayer = getPlayer(i);
            if (pPlayer->deathTime > 0 && pPlayer->fragger == pSpr)
                pPlayer->fragger = nullptr;

            if (pOpt->nGameType == kGameTypeCoop && pFrom->GetType() == pPlayer->GetActor()->GetType())
                pPlayer->fragCount++;
        }
    }

    trTriggerSprite(pSpr, kCmdOff, pFrom);
    pSpr->spr.flags |= (kPhysGravity | kPhysFalling | kPhysMove);

    switch (nDmgType)
    {
        case kDamageExplode:
            PlaySound(kCdudeSndDeathExplode);
            break;
        case kDamageBurn:
            sfxPlay3DSound(pSpr, 351, -1, 0);
            break;
    }

    if (IsBurning())
    {
        PlaySound(kCdudeSndDeathExplode);
        if (Chance(0x4000))
        {
            double top, bottom;
            GetActorExtents(pSpr, &top, &bottom);
            DVector3 gibPos(pSpr->spr.pos.XY(), top);
            DVector3 gibVel(pSpr->vel.XY() * 0.5, FixedToFloat(-0xccccc));
            GibSprite(pSpr, GIBTYPE_7, &gibPos, &gibVel);

            i = ClipLow(Random(3), 1);
            while (--i >= 0)
                fxSpawnBlood(pSpr, nDmg);
        }
    }
    else
    {
        PlaySound(kCdudeSndDeathNormal);
    }

    if (IsUnderwater())
        evPostActor(pSpr, 0, AF(EnemyBubble));

    if (nDmgType == kDamageExplode)
    {
        for (i = 0; i < countof(pInfo->nGibType); i++)
        {
            if (pInfo->nGibType[i] >= 0)
                GibSprite(pSpr, (GIBTYPE)pInfo->nGibType[i], nullptr, nullptr);
        }

        while (--i)
            fxSpawnBlood(pSpr, nDmg);
    }

    pSpr->ChangeType(kDudeModernCustom);
    if (AllowedKillType(pSpr)) Level.addKill(-1);
    pSpr->clipdist  = 1;
    pSpr->xspr.health   = 0;

    if (actCheckRespawn(pSpr))
    {
        // keep the dude statnum
        actPostSprite(pSpr, kStatDude);
        StatusSet(kCdudeStatusRespawn);
    }
    else
    {
        // fix double item drop
        pSpr->xspr.dropMsg = 0;
    }

    pSpr->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    pSpr->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
    seqKill(pSpr);
    pDeath = PickDeath(nDmgType);
    StatusSet(kCdudeStatusDying);
    NewState(pDeath);
}

void DCustomDude::DropItems(void)
{
    TArray<PClass*> itemList;
    DBloodActor* pDrop;
    
    if (dropItem.Pick(pSpr, itemList))
    {
        for(auto cls : itemList)
        {
            pDrop = actDropObject(pSpr, cls);
            if (pDrop && IsUnderwater()) // add a physics for drop items
            {
                pDrop->spr.pos.Z = pSpr->spr.pos.Z;
                pDrop->addX();

                pDrop->xspr.physAttr |= (kPhysGravity | kPhysFalling); // we only want it to drown/fall
                gPhysSpritesList.Push(MakeObjPtr(pDrop));
                getSpriteMassBySize(pDrop);
            }
        }
    }
}

void DCustomDude::ClearEffectCallbacks(void)
{
    int i = countof(gCdudeCustomCallback);
    while (--i >= 0)
        evKillActor(pSpr, *gCdudeCustomCallback[i]);
}

void DCustomDude::LeechPickup(void)
{
    DBloodActor* pLeech = this->pLeech;
    if (pLeech)
    {
        DBloodActor* pFX = gFX.fxSpawnActor((FX_ID)52, pLeech->sector(), pLeech->spr.pos, pLeech->spr.Angles.Yaw);
        if (pFX)
        {
            pFX->spr.cstat = CSTAT_SPRITE_ALIGNMENT_FACING;
            pFX->spr.scale.X = pFX->spr.scale.Y = 1 + Random(50) * REPEAT_SCALE;
            pFX->spr.pal = 6;
        }

        sfxPlay3DSoundVolume(pLeech, 490, -1, 0, 60000);
        actPostSprite(pLeech, kStatFree);
        this->pLeech = nullptr;
    }
}

void DCustomDude::LeechKill(bool delSpr)
{
    DBloodActor* pLeech = this->pLeech;
    if (pLeech)
    {
        pLeech->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
        pLeech->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
        pLeech->spr.flags = 0;

        // lock it, so we can detect it as killed after loading a save
        pLeech->xspr.health = 1;
        pLeech->xspr.locked = 1;
        pLeech->xspr.Proximity = 0;

        if (delSpr)
            actPostSprite(pLeech, kStatFree);
    }

    CUSTOMDUDE_WEAPON* pWeap; // set a zero ammo, so dude cannot use it
    for (int i = 0; i < numWeapons; i++)
    {
        pWeap = &weapons[i];
        if (pWeap->id == kModernThingEnemyLifeLeech)
        {
            pWeap->ammo.SetTotal(1);
            pWeap->ammo.Set(0);
        }
    }
}

void DCustomDude::UpdateSlaves()
{
    int t = 0;
    if (pSlaves.Size() == 0)
        return;

    for(int l = pSlaves.SSize() - 1; l >= 0; l--)
    {
        DBloodActor* pSlave = pSlaves[l];
        if (pSlave->IsDudeActor() && xsprIsFine(pSlave) && pSlave->ownerActor == pSpr)
        {
            if (pSlave->xspr.health > 0)
            {
                if (!pSpr->xspr.target)
                {
                    if (!pSlave->xspr.target)
                    {
                        // try return to master
                        aiSetTarget(pSlave, pSpr->spr.pos);
                    }
                    else
                    {
                        // call master!
                        DBloodActor* pTarget = pSlave->xspr.target;
                        switch (pSpr->xspr.aiState->stateType)
                        {
                            case kAiStateIdle:
                                aiSetTarget(pSpr, pTarget->spr.pos);
                                Activate();
                                break;
                        }
                    }
                }
                else if (pSpr->xspr.target != pSlave->xspr.target)
                {
                    // set same target
                    aiSetTarget(pSlave, pSpr->xspr.target);
                }
                continue;
            }
        }

        pSlaves.Delete(l);
    }

    if (version == 1 && t)
    {
        CUSTOMDUDE_WEAPON* pWeap;
        // add ammo for summon weapons
        for (int l = 0; l < numWeapons; l++)
        {
            pWeap = &weapons[l];
            if (pWeap->type == kCdudeWeaponSummon)
                pWeap->ammo.Inc(t);
        }
    }
}

bool DCustomDude::CanMove(sectortype* pSect, bool Crusher, bool Water, bool Uwater, bool /*Depth*/, double bottom, double floorZ)
{
    if (pSpr->xspr.health)
    {
        if (abs(floorZ - bottom) > (int)fallHeight)
        {
            if (!Uwater)
                return false;
        }

        if (Water || Uwater)
        {
            if (!CanSwim())
                return false;
        }
        else if (Crusher && pSect && pSect->hasX()) // !nnExtIsImmune(pSpr, pSect->xs().damageType))
            return false;
    }

    return true;
}


/*************************************************************************************************/
const PARAM* CUSTOMDUDE_SETUP::FindParam(int nParam, const PARAM* pDb)
{
    const PARAM* ptr = pDb;
    while (ptr->id != kParamMax)
    {
        if (ptr->id == nParam)
            return ptr;

        ptr++;
    }

    return nullptr;
}

int CUSTOMDUDE_SETUP::FindParam(const char* str, const PARAM* pDb)
{
    const PARAM* ptr = pDb;
    while (ptr->id != kParamMax)
    {
        if (stricmp(str, ptr->text) == 0)
            return ptr->id;

        ptr++;
    }

    return -1;
}

int CUSTOMDUDE_SETUP::ParamLength(const PARAM* pDb)
{
    int i = 0;  
    const PARAM* pParam = pDb;
    while (pParam->id != kParamMax)
        pParam++, i++;

    return i;
}


const char* CUSTOMDUDE_SETUP::DescriptGetValue(const char* pGroupName, const char* pParamName)
{
    const char* pRetn = pIni->GetKeyString(pGroupName, pParamName, nullptr);
    if (isempty(pRetn)) // zero length strings leads to nullptr
        pRetn = nullptr;

    return pRetn;
}

bool CUSTOMDUDE_SETUP::DescriptGroupExist(const char* pGroupName)
{
    return pIni->SectionExists(pGroupName);
}

bool CUSTOMDUDE_SETUP::DescriptParamExist(const char* pGroupName, const char* pParamName)
{
    return pIni->KeyExists(pGroupName, pParamName);
}


bool CUSTOMDUDE_SETUP::DescriptLoad(int nID)
{   
    if (rngok(nID, 0, 10000))
    {
        // first try with path, then without.
        FStringf tmp("dudes/" kCdudeFileNamePrefix "%d." kCdudeFileExt, nID);
        const char* tname = tmp.GetChars();
        hIni = fileSystem.CheckNumForFullName(tname);
        if (hIni == -1) hIni = fileSystem.CheckNumForFullName(tname += 6);
        if (hIni == -1) return false;

        auto pRawIni = fileSystem.ReadFile(hIni);

        if (pRawIni.GetSize() > 0)
        {
            pIni.reset(new IniFile(tname));
            return true;
        }
    }

    return false;
}

void CUSTOMDUDE_SETUP::DescriptClose(void)
{
    if (pIni)
        pIni = nullptr;
}

int CUSTOMDUDE_SETUP::DescriptCheck(void)
{
    int nRetn = 0; char major[16];
    pGroup = FindParam(kParGroupGeneral, gParGroup);
    pParam = FindParam(kParGeneralVersion, gParamGeneral);
    pValue = nullptr;

    if (pIni)
    {
        if (!DescriptGroupExist(pGroup->text))
        {
            Warning(GetError(kErrReqGroupNotFound), pGroup->text);
            return nRetn;
        }
        
        pValue = DescriptGetValue(pGroup->text, pParam->text);
        if (pValue && rngok((int)strlen(pValue), 0, 5))
        {
            major[0] = pValue[0];
            major[1] = '\0';
            if (isdigit(major[0]))
                nRetn = atoi(major);
        }
        
        pGroup = FindParam(kParGroupParser, gParGroup);
        pParam = FindParam(kParParserWarnings, gParamParser);
        if (pGroup && pParam)
        {
            pValue = DescriptGetValue(pGroup->text, pParam->text);
            if (isbool(pValue))
                showWarnings = btoi(pValue);
        }
    }

    return nRetn;
}

void CUSTOMDUDE_SETUP::SetupSlaves()
{
    DBloodActor* pSpr = pDude->pSpr;

    pDude->pSlaves.Clear();

    BloodStatIterator it(kStatDude);
    while (auto pSpr2 = it.Next())
    {
        if (pSpr2->ownerActor == pSpr && pSpr2->IsDudeActor() && xsprIsFine(pSpr2))
        {
            pDude->pSlaves.Push(MakeObjPtr(pSpr2));
        }
    }
}

void CUSTOMDUDE_SETUP::SetupLeech()
{
    DBloodActor* pSpr = pDude->pSpr;

    pDude->pLeech = nullptr;
    BloodStatIterator it(kStatThing);
    while (auto pSpr2 = it.Next())
    {
        if (pSpr2->ownerActor == pSpr && pSpr2->hasX())
        {
            if (pSpr2->xspr.locked)
                pDude->LeechKill(false); // repeat fake killing to set 0 ammo

            // found!
            pDude->pLeech = pSpr2;
            break;
        }
    }
}

DCustomDude* CUSTOMDUDE_SETUP::GetFirstDude(int nID)
{
    DCustomDude* pRetn;
    BloodStatIterator it(kStatDude);
    while (auto pSpr2 = it.Next())
    {
        pRetn = cdudeGet(pSpr2);
        if (pRetn->initialized && pRetn->pSpr->xspr.data1 == nID)
            return pRetn;
    }

    return nullptr;
}

bool CUSTOMDUDE_SETUP::IsFirst(DCustomDude* pCmp)
{
    return (GetFirstDude(pCmp->pSpr->xspr.data1) == pCmp);
}

DCustomDude* CUSTOMDUDE_SETUP::SameDudeExist(DCustomDude* pCmp)
{
    BloodStatIterator it(kStatDude);
    while (auto pSpr2 = it.Next())
    {
        auto pOther = pSpr2->customDude;
        if (pOther != pCmp && pOther->initialized && pOther->version == pCmp->version)
        {
            if (pOther->pSpr->xspr.data1 == pCmp->pSpr->xspr.data1)
                return pOther;
        }
    }

    return nullptr;
}

void CUSTOMDUDE_SETUP::RandomizeDudeSettings()
{
    // randomize timers of some states, so dudes look more or less unique
    const int states[] = { kCdudeStateSearch, kCdudeStateKnock, kCdudeStateFlee };
    int nTime, nVal, i, j;

    for (i = 0; i < countof(states); i++)
    {
        AISTATE* pState = pDude->states[states[i]];
        for (j = 0; j < kCdudePostureMax; j++)
        {
            nTime = pState->stateTicks;
            if (nTime)
            {
                nVal = perc2val(20, nTime);
                pState->stateTicks = ClipLow(nTime + Random2(nVal), 0);
            }

            pState++;
        }
    }
}

bool CUSTOMDUDE_SETUP::FindAiState(AISTATE stateArr[][kCdudePostureMax], int arrLen, AISTATE* pNeedle, int* nType, int* nPosture)
{
    int i, j;
    for (i = 0; i < arrLen; i++)
    {
        for (j = 0; j < kCdudePostureMax; j++)
        {
            if (pNeedle == &stateArr[i][j])
            {
                *nType = i; *nPosture = j;
                return true;
            }
        }
    }

    return false;
}

void CUSTOMDUDE_SETUP::Setup(DBloodActor* pSpr)
{
    setup.DoSetup(pSpr);
}

void CUSTOMDUDE_SETUP::DoSetup(DBloodActor* pSpr)
{
    AISTATE* pModel, * pState;
    int nStateType, nPosture;
    int i, j;
    
    DCustomDude* pDude = Create<DCustomDude>();
    if (pSpr->customDude) pSpr->customDude->Destroy(); // if we already got one, destroy it to start fresh.
    pSpr->customDude = pDude;
    GC::WriteBarrier(pSpr, pDude);

    pDude->version = 0;
    pIni = nullptr;
    hIni = 0;

    pDude->pInfo = getDudeInfo(pSpr);
    pDude->pSpr = pSpr;
    pDude->pExtra = &pSpr->dudeExtra;
    pDude->pLeech = nullptr;

    pDude->pWeapon  = &pDude->weapons[0];
    pDude->posture  = kCdudePostureL;
    pDude->nextDude = false;

    // default stuff
    pDude->_seeDist      = pDude->pInfo->SeeDist();
    pDude->_hearDist     = pDude->pInfo->HearDist();
    pDude->periphery    = pDude->pInfo->Periphery();
    pDude->fallHeight   = INT32_MAX;

    // copy general states
    for (i = 0; i < kCdudeStateNormalMax; i++)
    {
        for (j = 0; j < kCdudePostureMax; j++)
        {
            pModel = &gCdudeStateTemplate[i][j];
            pState = &pDude->states[i][j];

            memcpy(pState, pModel, sizeof(AISTATE));

            if (pModel->nextState)
            {
                // fix next state pointers after copying
                if (FindAiState(gCdudeStateTemplate, countof(gCdudeStateTemplate), pModel->nextState, &nStateType, &nPosture))
                    pState->nextState = &pDude->states[nStateType][nPosture];
                else
                    pState->nextState = nullptr;
            }
        }
    }

    // copy dying states
    pModel = gCdudeStateDyingTemplate;
    for (i = kCdudeStateDeathBase; i < kCdudeStateDeathMax; i++)
    {
        for (j = 0; j < kCdudePostureMax; j++)
        {
            pState = &pDude->states[i][j];
            memcpy(pState, pModel, sizeof(AISTATE));
        }
    }

    // copy weapon attack states
    pModel = gCdudeStateAttackTemplate;
    for (i = kCdudeStateAttackBase; i < kCdudeStateAttackMax; i++)
    {
        for (j = 0; j < kCdudePostureMax; j++)
        {
            pState = &pDude->states[i][j];
            memcpy(pState, pModel, sizeof(AISTATE));
            pState->nextState = pState;
        }
    }

    DoSetup();
    SetupLeech();
    SetupSlaves();
    
    if (!IsFirst(pDude))
        RandomizeDudeSettings();

    pDude->initialized = 1;
}


void CUSTOMDUDE_SETUP::Setup(DCustomDude* pOver)
{
    setup.DoSetup(pOver);
}

void CUSTOMDUDE_SETUP::DoSetup(DCustomDude* pOver)
{
    if (pOver)
        pDude = pOver;

    assert(pDude && pDude->pSpr->hasX());
    DBloodActor* pSpr = pDude->pSpr;
    int nPrevVer = pDude->version;
    int nVer;

    nWarnings    = 0;
    showWarnings = true;

    pDude->version = kCdudeVer1;
    if (DescriptLoad(pSpr->xspr.data1))
    {
        nVer = DescriptCheck();
        if (nVer == kCdudeVer2)
        {
            pDude->version = kCdudeVer2;
        }
        else
        {
            Warning(GetError(kErrInvalidVersion));
            DescriptClose();

            pSpr->xspr.data1 = 0;
            pSpr->xspr.data2 = 0;
            pSpr->xspr.data3 = 0;
        }
    }

    if (nPrevVer)
    {
        pDude->ClearEffectCallbacks();
        if (nPrevVer != pDude->version)
        {
            if (pDude->version == kCdudeVer2)
                DescriptClose();
            
            // do a full re-init
            Setup(pDude->pSpr);
            return;
        }
    }

    pDude->dropItem.Clear();

    switch (pDude->version)
    {
        case kCdudeVer2:
            static_cast<CUSTOMDUDEV2_SETUP*>(&setup)->Setup();
            DescriptClose();
            break;
        default:
            static_cast<CUSTOMDUDEV1_SETUP*>(&setup)->Setup();
            break;
    }
    
    AnimationFill();
    FindLargestPic();
    SoundFill();
}

void CUSTOMDUDE_SETUP::Warning(const char* pFormat, ...)
{
    int i = kErrMax;
    FString buffer;

    if (!SameDudeExist(pDude) && showWarnings)
    {
        buffer += "\n";
        while (--i >= 0)
        {
            if (gErrors[i] == pFormat)
            {
                buffer.AppendFormat("CDUD_WARN#%d", ++nWarnings);
                break;
            }
        }

        buffer += " ";

        if (hIni)
        {
            buffer.AppendFormat("in file '%s'", fileSystem.GetFileFullName(hIni));
        }

        buffer += " ";

        va_list args;
        va_start(args, pFormat);
        buffer.VAppendFormat(pFormat, args);
        va_end(args);

        buffer += ".\n";

        if (pGroup)
        {
            buffer.AppendFormat("Group: %s", pGroup->text);
            if (pIni)
            {
                buffer.AppendFormat(" (%s)", DescriptGroupExist(pGroup->text) ? "exist" : "not exist");
            }

            buffer.AppendFormat(", Parameter: %s", pParam->text);
            if (pIni)
            {
                buffer.AppendFormat(" (%s)", DescriptParamExist(pGroup->text, pParam->text) ? "exist" : "not exist");
            }

            buffer.AppendFormat("\n");
            buffer.AppendFormat("\nValue: \"%s\"\n\n", (pValue) ? pValue : "<empty>");
        }

        Printf(PRINT_HIGH, "%s", buffer.GetChars());
        if (nWarnings == 1)
            sndStartSample(778, 255);
    }
}

const char* CUSTOMDUDE_SETUP::GetValType(int nType)
{
    nType = ClipRange(nType, 0, countof(gValTypes) - 1);
    return gValTypes[nType];
}

const char* CUSTOMDUDE_SETUP::GetError(int nErr)
{
    nErr = ClipRange(nErr, 0, countof(gErrors) - 1);
    return gErrors[nErr];
}

void CUSTOMDUDE_SETUP::VelocitySetDefault(int nMaxVel)
{
    DUDEINFO* pInfo = pDude->pInfo; 
    DBloodActor* pSpr = pDude->pSpr;
    int nFrontSpeed = pInfo->frontSpeed;
    int nSideSpeed = pInfo->sideSpeed;
    int nAngSpeed = pInfo->angSpeed;
    int i;

    if (pSpr->xspr.busyTime)
        nFrontSpeed = ClipHigh((nFrontSpeed / 3) + (2500 * pSpr->xspr.busyTime), nMaxVel);

    for (i = 0; i < kCdudePostureMax; i++)
    {
        pDude->velocity[i].Set(nFrontSpeed, kParVelocityForward);
        pDude->velocity[i].Set(nSideSpeed, kParVelocityDodge);
        pDude->velocity[i].Set(nAngSpeed, kParVelocityTurn);
    }
}

void CUSTOMDUDE_SETUP::DamageSetDefault(void)
{
    for (int i = 0; i < kDmgMax; i++)
        pDude->damage.id[i] = getDudeInfo(kDudeModernCustom)->startDamage[i];
}

void CUSTOMDUDE_SETUP::DamageScaleToSkill(int nSkill)
{
    CUSTOMDUDE_DAMAGE* pDmg = &pDude->damage;
    for (int i = 0; i < kDmgMax; i++)
        pDmg->Set(MulScale(DudeDifficulty[nSkill], pDmg->id[i], 8), i);
}

void CUSTOMDUDE_SETUP::WeaponDispersionSetDefault(CUSTOMDUDE_WEAPON* pWeapon)
{
    int nDisp = ClipLow((3000 / (gGameOptions.nDifficulty + 1)) + Random2(500), 500);
    pWeapon->dispersion[0] = nDisp;
    pWeapon->dispersion[1] = nDisp >> 1;
}

void CUSTOMDUDE_SETUP::WeaponRangeSet(CUSTOMDUDE_WEAPON* pWeapon, int nMin, int nMax)
{
    pWeapon->distRange[0] = nMin * inttoworld, pWeapon->distRange[1] = nMax * inttoworld;
    if (nMin > nMax)
        std::swap(pWeapon->distRange[0], pWeapon->distRange[1]);
}

inline int IncRotate(int n, int mod)
{
    if (++n >= mod)
        n = 0;
    return n;
}

void CUSTOMDUDE_SETUP::WeaponSoundSetDefault(CUSTOMDUDE_WEAPON* pWeapon)
{
    int i, j = 0;
    CUSTOMDUDE_SOUND* pSound = &pWeapon->sound;
    for (i = 0; i < kCdudeMaxSounds; i++)
    {
        switch (pWeapon->type)
        {
            case kCdudeWeaponHitscan:
            {
                const VECTORINFO_EXTRA* pExtra = &gVectorInfoExtra[pWeapon->id];
                pSound->id[i] = soundEngine->FindSoundByResID(pExtra->fireSound[j]);
                j = IncRotate(j, countof(pExtra->fireSound));
                break;
            }
            case kCdudeWeaponMissile:
            {
                const MISSILEINFO_EXTRA* pExtra = &gMissileInfoExtra[pWeapon->id - kMissileBase];
                pSound->id[i] = soundEngine->FindSoundByResID(pExtra->fireSound[j]);
                j = IncRotate(j, countof(pExtra->fireSound));
                break;
            }
            case kCdudeWeaponKamikaze:
            {
                const EXPLOSION_EXTRA* pExtra = &gExplodeExtra[pWeapon->id - kTrapExploder];
                pSound->id[i] = soundEngine->FindSoundByResID(pExtra->snd);
                break;
            }
            case kCdudeWeaponThrow:
                pSound->id[i] = soundEngine->FindSoundByResID(455);
                break;
            case kCdudeWeaponSummon:
                pSound->id[i] = soundEngine->FindSoundByResID(379);
                break;
        }
    }
}

void CUSTOMDUDE_SETUP::AnimationConvert(int baseID)
{
    const SEQCOMPAT* pEntry;
    AISTATE* pState;

    int i, j, nSeq;
    for (i = 0; i < kCdudeStateMax; i++)
    {
        pState = pDude->states[i];
        AnimationFill(pState, 0);
    }

    for (i = 0; i < countof(gSeqCompat); i++)
    {
        pEntry = &gSeqCompat[i];
        if (pEntry->nAiStateType >= 0)
        {
            pState = pDude->states[pEntry->nAiStateType];
            for (j = 0; j < kCdudePostureMax; j++)
            {
                nSeq = baseID + pEntry->offset[j];
                pState[j].seqId = getSequence(nSeq) ? nSeq : 0;
            }
        }
    }
}

void CUSTOMDUDE_SETUP::AnimationFill(AISTATE* pState, int nAnim)
{
    for (int i = 0; i < kCdudePostureMax; i++) pState[i].seqId = nAnim;
}

void CUSTOMDUDE_SETUP::AnimationFill(void)
{
    AISTATE* pState;
    int i, j;

    for (i = 0; i < kCdudeStateMax; i++)
    {
        for (j = 0; j < kCdudePostureMax; j++)
        {
            pState = &pDude->states[i][j];
            if (!getSequence(pState->seqId))
                pState->seqId = 0;
        }
    }
}

void CUSTOMDUDE_SETUP::SoundConvert(int baseID)
{
    CUSTOMDUDE_WEAPON* pWeap;
    CUSTOMDUDE_SOUND* pSound;
    unsigned int i, j, k;
    const SNDCOMPAT* pEntry;

    // fill with default sounds first
    memcpy(pDude->sound, gSoundTemplate, sizeof(gSoundTemplate));
    for (i = 0; i < pDude->numWeapons; i++)
    {
        pWeap = &pDude->weapons[i];
        WeaponSoundSetDefault(pWeap);
    }

    if (baseID > 0)
    {
        for (i = 0; i < countof(gSndCompat); i++)
        {
            pEntry = &gSndCompat[i];
            if (irngok(pEntry->nSoundType, kCdudeSndCompatAttack1, kCdudeSndCompatAttack3))
            {
                switch (pEntry->nSoundType)
                {
                    case kCdudeSndCompatAttack3:                        // melee attack
                        pWeap = &pDude->weapons[1];                     // it's a second weapon now
                        break;
                    case kCdudeSndCompatAttack2:                        // throw weapon
                        pWeap = &pDude->weapons[0];
                        if (pWeap->type == kCdudeWeaponThrow) break;
                        else continue;                                  // no way to convert it?
                    default:                                            // normal attack
                        pWeap = &pDude->weapons[0];
                        break;
                }

                pSound = &pWeap->sound;

                // setup sound properties
                pSound->medium          = 0x03;
                pSound->interruptable   = 0x01;
                pSound->ai              = 0x00;
                pSound->once            = 0x00;

                // the weapon uses default sounds
                if (soundEngine->FindSoundByResID(baseID + pEntry->offset) == NO_SOUND)
                    continue;
            }
            else
            {
                pSound = &pDude->sound[pEntry->nSoundType];
            }

            j = 0, k = 0; // now, fill
            while (j < kCdudeMaxSounds)
            {
                int nSnd = baseID + k + pEntry->offset;
                k = IncRotate(k, pEntry->range);
                pSound->id[j++] = soundEngine->FindSoundByResID(nSnd);
            }
        }
    }
}

void CUSTOMDUDE_SETUP::SoundFill(CUSTOMDUDE_SOUND* pSound, int nSnd)
{
    for (int i = 0; i < kCdudeMaxSounds; i++) pSound->id[i] = soundEngine->FindSoundByResID(nSnd);
}

void CUSTOMDUDE_SETUP::SoundFill(void)
{
    CUSTOMDUDE_SOUND* pSnd; int i, j;
    FSoundID nSnd;

    // fill dude sounds
    for (i = 0; i < kCdudeSndMax; i++)
    {
        pSnd = pDude->sound;
        for (j = 0, nSnd = NO_SOUND; j < kCdudeMaxSounds; j++)
        {
            if (nSnd == NO_SOUND)
            {
                if (pSnd->id[j] != NO_SOUND)
                {
                    nSnd = pSnd->id[j];
                    j = 0;
                }
            }
            else if (pSnd->id[j] == NO_SOUND)
            {
                pSnd->id[j] = nSnd;
            }
        }
    }

    // fill weapon sounds
    for (i = 0; i < kCdudeMaxWeapons; i++)
    {
        pSnd = &pDude->weapons[i].sound;
        for (j = 0, nSnd = NO_SOUND; j < kCdudeMaxSounds; j++)
        {
            if (nSnd == NO_SOUND)
            {
                if (pSnd->id[j] != NO_SOUND)
                {
                    nSnd = pSnd->id[j];
                    j = 0;
                }
            }
            else if (pSnd->id[j] != NO_SOUND)
            {
                pSnd->id[j] = nSnd;
            }
        }
    }
}

void CUSTOMDUDE_SETUP::FindLargestPic(void)
{
    int i, j, nHeigh = 0;
    AISTATE* pState; Seq* pSeq;

    for (i = 0; i < kCdudeStateMax; i++)
    {
        pState = &pDude->states[i][kCdudePostureL];
        if (pState->seqId <= 0 || (pSeq = getSequence(pState->seqId)) == nullptr)
            continue;

        for (j = 0; j < pSeq->nFrames; j++)
        {
            auto nPic = seqGetTexture(&pSeq->frames[j]);
            auto pPic = TexMan.GetGameTexture(nPic);
            int h = (int)pPic->GetDisplayHeight();
            if (h > nHeigh)
            {
                nHeigh = h;
                pDude->largestPic = nPic;
            }
        }
    }
}


/*************************************************************************************************/


int CUSTOMDUDEV2_SETUP::CheckRange(const char* str, int nVal, int nMin, int nMax)
{
    if (!irngok(nVal, nMin, nMax))
    {
        Warning(GetError(kErrInvalidRange), str, nMin, nMax);
        return nMin;
    }

    return nVal;
}

int CUSTOMDUDEV2_SETUP::CheckValue(const char* str, int nValType, int nDefault)
{
    int t = 0;
    bool ok = false;
    
    if (rngok(nValType, kValIdKeywordBase, kValIdKeywordMax))
    {
        if (isIdKeyword(str, GetValType(nValType), &t))
            return t;
    }
    else
    {
        switch (nValType)
        {
            case kValFix:       ok = isfix(str);                                    break;
            case kValUfix:      ok = isufix(str);                                   break;
            case kValPerc:      ok = isperc(str);                                   break;
            case kValBool:      ok = isbool(str);                                   break;
            case kValArrC:      ok = isarray(str, &t);                              break;
            case kValArrA:      ok = isarray(str, &t);                              break;
        }
    }

    if (!ok)
    {
        Warning(GetError(kErrInvalidValType), str, GetValType(nValType));
        return nDefault;
    }

    switch (nValType)
    {
        case kValArrC:
        case kValArrA:
            return t;
        case kValBool:
            return btoi(str);
        default:
            return atoi(str);
    }
}

int CUSTOMDUDEV2_SETUP::CheckValue(const char* str, int nValType, int nMin, int nMax, int nDefault)
{
    int nRetn = CheckValue(str, nValType, INT32_MAX);
    if (nRetn == INT32_MAX)
        return nDefault;

    if (!irngok(nRetn, nMin, nMax))
    {
        Warning(GetError(kErrInvalidRange), str, nMin, nMax);
        return nDefault;
    }

    return nRetn;
}

int CUSTOMDUDEV2_SETUP::CheckValue(const char* str, int nValType, int nMin, int nMax)
{
    int nRetn = CheckValue(str, nValType, INT32_MAX);
    if (nRetn != INT32_MAX)
        return CheckRange(str, nRetn, nMin, nMax);

    return nMin;
}

int CUSTOMDUDEV2_SETUP::CheckParam(const char* str, const PARAM* pDb)
{
    int nRetn = FindParam(str, pDb);
    if (nRetn < 0)
        Warning(GetError(kErrInvalidParam), str);

    return nRetn;
}

int CUSTOMDUDEV2_SETUP::ParseKeywords(const char* str, const PARAM* pDb)
{
    char tmp[256];
    int nRetn = 0, nPar, i;
    int dLen = ParamLength(pDb);

    i = 0;
    while (i < dLen && enumStr(i++, str, tmp))
    {
        if ((nPar = FindParam(tmp, pDb)) >= 0)
        {
            nRetn |= nPar;
        }
        else
        {
            Warning(GetError(kErrInvalidParam), tmp);
        }
    }

    return nRetn;
}

void CUSTOMDUDEV2_SETUP::SetupGeneral(void)
{
    int nVal = 0, i, range[2];
    DBloodActor* pSpr = pDude->pSpr;
    const PARAM* pMorph;

    /* ----------------------------------*/
    /* DEFAULT VALUES                    */
    /* ----------------------------------*/
    pDude->mass         = 75;
    pDude->medium       = kParMediumAny;
    pDude->nextDude = false;

    pParam = gParamGeneral;
    while (pParam->id != kParamMax)
    {
        pValue = DescriptGetValue(pGroup->text, pParam->text);
        if (pValue)
        {
            switch (pParam->id)
            {
                case kParGeneralActiveTime:
                    nVal = CheckValue(pValue, kValUfix, 0, 32767, 800);
                    for (i = 0; i < kCdudePostureMax; i++) pDude->states[kCdudeStateSearch][i].stateTicks = nVal;
                    break;
                case kParGeneralMass:
                    pDude->mass = CheckValue(pValue, kValUfix, 1, 65535, 75);
                    break;
                case kParGeneralMedium:
                    if ((nVal = ParseMedium(pValue)) >= 0)
                    {
                        pDude->medium = nVal;
                        break;
                    }
                    Warning(GetError(kErrInvalidResultC), pValue, pParam->text);
                    break;
                case kParGeneralHealth:
                    if (!pDude->initialized)
                    {
                        if (!pSpr->xspr.sysData2)
                        {
                            nVal = CheckValue(pValue, kValUfix, 1, 65535, 60);
                            pSpr->xspr.health = nnExtDudeStartHealth(pSpr, nVal);
                        }
                        else
                        {
                            pSpr->xspr.health = nnExtDudeStartHealth(pSpr, pSpr->xspr.sysData2);
                        }
                    }
                    break;
                case kParGeneralClipdist:
                    pSpr->clipdist = CheckValue(pValue, kValUfix, 0, 255, 48);
                    break;
                case kParGeneralMorphTo:
                    for (pMorph = gParamMorphTo; pMorph->text; pMorph++)
                    {
                        if (isIdKeyword(pValue, pMorph->text, &nVal))
                        {
                            switch (pMorph->id)
                            {
                                case kValCdud:
                                    pDude->nextDude = CheckRange(pMorph->text, nVal, 0, 9999);
                                    break;
                                case kValVdud:
                                    range[0] = kDudeCultistTommy - kDudeBase;
                                    range[1] = kDudeVanillaMax - kDudeBase;
                                    pDude->nextDude = GetSpawnType(kDudeBase + CheckRange(pMorph->text, nVal, range[0], range[1]));
                                    break;
                                default:
                                    continue;
                            }

                            break;
                        }
                    }

                    if (!pMorph->text)
                        Warning(GetError(kErrInvalidResultC), pValue, pParam->text);

                    break;
            }
        }

        pParam++;
    }
}


void CUSTOMDUDEV2_SETUP::SetupVelocity(void)
{
    CUSTOMDUDE_VELOCITY* pVel;
    VelocitySetDefault(kCdudeMaxVelocity);
    pParam = gParamPosture;

    if (DescriptGroupExist(pGroup->text))
    {
        while (pParam->id != kParamMax)
        {
            if (rngok(pParam->id, kCdudePosture, kCdudePostureMax))
            {
                pVel = &pDude->velocity[pParam->id - kCdudePosture];
                pValue = DescriptGetValue(pGroup->text, pParam->text);
                ParseVelocity(pValue, pVel);
            }

            pParam++;
        }
    }
}

void CUSTOMDUDEV2_SETUP::SetupAnimation(AISTATE* pState, bool asPosture)
{
    AnimationFill(pState, 0); // clear seqID first
    ParseAnimation(pValue, pState, asPosture);
}

void CUSTOMDUDEV2_SETUP::SetupAnimation(void)
{
    int range[2];

    /* ----------------------------------*/
    /* DEFAULT VALUES                    */
    /* ----------------------------------*/
    pDude->pSpr->xspr.scale = kCdudeDefaultAnimScale;

    if (!DescriptGroupExist(pGroup->text))
    {
        // whole group is not found, so just use v1 animation set
        AnimationConvert(kCdudeDefaultSeq);
        return;
    }

    pParam = gParamAnim;
    while (pParam->id != kParamMax)
    {
        pValue = DescriptGetValue(pGroup->text, pParam->text);

        switch (pParam->id)
        {
            case kCdudeAnimScale:
                range[0] = 0; range[1] = 1024;
                switch (ParseRange(pValue, kValPerc, range, kCdudeDefaultAnimScale))
                {
                    case 1: pDude->pSpr->xspr.scale = range[0];                                 break;
                    case 2: pDude->pSpr->xspr.scale = range[0] + Random2(range[1] - range[0]);  break;
                }
                break;
            case kCdudeStateMove:
                for (int i = kCdudeStateMoveBase; i < kCdudeStateMoveMax; i++)
                {
                    const PARAM* pMove = FindParam(i, gParamAnim);
                    if (!pMove || !DescriptGetValue(pGroup->text, pMove->text))
                        SetupAnimation(pDude->states[i], true);
                }
                break;
            default:
                if (rngok(pParam->id, kCdudeStateBase, kCdudeStateNormalMax))
                {
                    SetupAnimation(pDude->states[pParam->id], true);
                    switch (pParam->id)
                    {
                        case kCdudeStateIdle:
                            SetupAnimation(pDude->states[kCdudeStateGenIdle], true); // ai fight compat
                            break;
                    }
                }
                else if (rngok(pParam->id, kCdudeStateDeathBase, kCdudeStateDeathMax))
                {
                    AISTATE* pState = pDude->states[pParam->id];
                    SetupAnimation(pState, false);
                }
                break;
        }

        pParam++;
    }
}

void CUSTOMDUDEV2_SETUP::SetupSound(CUSTOMDUDE_SOUND* pSound)
{
    SoundFill(pSound, 0); // clear rawIDs first
    ParseSound(pValue, pSound);
}

void CUSTOMDUDEV2_SETUP::SetupSound(void)
{
    CUSTOMDUDE_SOUND* pSound;

    if (!DescriptGroupExist(pGroup->text))
    {
        // whole group is not found, so just use v1 sound set
        SoundConvert(0);
        return;
    }

    
    for (pParam = gParamSounds; pParam->id != kParamMax; pParam++)
    {
        if (rngok(pParam->id, kCdudeSnd, kCdudeSndMax))
        {
            pSound = &pDude->sound[pParam->id];
            pValue = DescriptGetValue(pGroup->text, pParam->text);
            memcpy(pSound, &gSoundTemplate[pParam->id], sizeof(CUSTOMDUDE_SOUND));
            SetupSound(pSound);
        }
    }
}

void CUSTOMDUDEV2_SETUP::SetupDamage(void)
{
    CUSTOMDUDE_DAMAGE* pDamage = &pDude->damage;
    int nVal, t;
    
    /* ----------------------------------*/
    /* DEFAULT VALUES                    */
    /* ----------------------------------*/
    DamageSetDefault();

    if (DescriptGroupExist(pGroup->text))
    {
        pParam = gParamDamage;
        while (pParam->id != kParamMax)
        {
            pValue = DescriptGetValue(pGroup->text, pParam->text);
            if (pValue)
            {
                switch (pParam->id)
                {
                    case kParDmgSource:
                        pDamage->ignoreSources = ParseKeywords(pValue, gParamDamageSource);
                        break;
                    default:
                        if (rngok(pParam->id, kDmgFall, kDmgMax))
                        {
                            t = pDamage->id[pParam->id];
                            nVal = CheckValue(pValue, kValPerc, 0, kCdudeMaxDmgScale, 100);
                            pDamage->Set(perc2val(nVal, t), pParam->id);
                        }
                        break;
                }
            }
            
            pParam++;
        }
    }

    DamageScaleToSkill(gGameOptions.nDifficulty);

}

void CUSTOMDUDEV2_SETUP::SetupRecoil(void)
{
    CUSTOMDUDE_RECOIL* pRecoil = &pDude->recoil;
    int onEventDmg[2];

    /* ----------------------------------*/
    /* DEFAULT VALUES                    */
    /* ----------------------------------*/
    memset(pRecoil, 0, sizeof(CUSTOMDUDE_RECOIL));

    if (!DescriptGroupExist(pGroup->text))
        return;

    pParam = gParamRecoil;
    while (pParam->id != kParamMax)
    {
        pValue = DescriptGetValue(pGroup->text, pParam->text);
        if (pValue)
        {
            switch (pParam->id)
            {
                case kParEventOnDmg:
                    ParseOnEventDmg(pValue, onEventDmg, 3);
                    pRecoil->dmgReq = onEventDmg[0];
                    pRecoil->chance = onEventDmg[1];


                    break;
            }
        }

        pParam++;
    }

}

void CUSTOMDUDEV2_SETUP::SetupDodge(void)
{
    CUSTOMDUDE_DODGE* pDodge = &pDude->dodge;
    int onEventDmg[2];
    int nVal;

    /* ----------------------------------*/
    /* DEFAULT VALUES                    */
    /* ----------------------------------*/
    memset(pDodge, 0, sizeof(CUSTOMDUDE_DODGE));
    pDodge->onAimMiss.chance = 0x10000;

    if (!DescriptGroupExist(pGroup->text))
        return;

    pParam = gParamDodge;
    while (pParam->id != kParamMax)
    {
        pValue = DescriptGetValue(pGroup->text, pParam->text);
        if (pValue)
        {
            switch (pParam->id)
            {
                case kParEventOnDmg:
                    ParseOnEventDmg(pValue, onEventDmg, 2);
                    pDodge->onDamage.dmgReq = onEventDmg[0];
                    pDodge->onDamage.chance = onEventDmg[1];
                    break;
                case kParEventOnAimTargetWrong:
                    nVal = CheckValue(pValue, kValPerc, 0, 100);
                    pDodge->onAimMiss.chance = perc2val(nVal, 0x10000);
                    break;
            }
        }

        pParam++;
    }
}



void CUSTOMDUDEV2_SETUP::SetupKnockout(void)
{
    CUSTOMDUDE_KNOCKOUT* pKnock = &pDude->knockout;
    int onEventDmg[3];
    AISTATE* pState;
    int i;

    /* ----------------------------------*/
    /* DEFAULT VALUES                    */
    /* ----------------------------------*/
    memset(pKnock, 0, sizeof(CUSTOMDUDE_KNOCKOUT));
    for (i = 0; i < kCdudePostureMax; i++)
    {
        pState = &pDude->states[kCdudeStateKnock][i];
        pState->stateTicks = 0;
    }

    if (!DescriptGroupExist(pGroup->text))
        return;
   
    pParam = gParamKnockout;
    while(pParam->id != kParamMax)
    {
        pValue = DescriptGetValue(pGroup->text, pParam->text);
        if (pValue)
        {
            switch (pParam->id)
            {
                case kParEventOnDmg:
                    ParseOnEventDmg(pValue, onEventDmg, 3);
                    pKnock->dmgReq      = onEventDmg[0];
                    pKnock->chance      = onEventDmg[1];
                    for (i = 0; i < kCdudePostureMax; i++)
                    {
                        pState = &pDude->states[kCdudeStateKnock][i];
                        pState->stateTicks = onEventDmg[2];
                    }
                    break;
            }
        }

        pParam++;
    }
}

int CUSTOMDUDEV2_SETUP::ParseIDs(const char* str, int nValType, TArray<int>& pOut, int nMax)
{
    char tmp[256];
    int i, nVal, nLen;

    if (isarray(str, &nLen))
    {
        if (nMax > 0 && nMax < nLen)
        {
            Warning(GetError(kErrInvalidArrayLen2), 1, nMax, nLen);
            return 0;
        }

        nMax = nLen;
    }

    i = 0;
    while ((nMax <= 0 || i < nMax) && enumStr(i, str, tmp))
    {
        if ((nVal = CheckValue(tmp, nValType, INT32_MAX)) != INT32_MAX)
            pOut.Push(nVal);

        i++;
    }

    return pOut.Size();
}

int CUSTOMDUDEV2_SETUP::ParseIDs(const char* str, int nValType, int* pOut, int nMax)
{
    int i = 0;
    TArray<int> ids(nMax, false);
    if (ParseIDs(str, nValType, ids, nMax))
    {
        i = min(nMax, ids.SSize());
        memcpy(pOut, ids.Data(), sizeof(int) * i);
    }
    return i;
}

int CUSTOMDUDEV2_SETUP::ParseEffectIDs(const char* str, const char* paramName, unsigned short* pOut, int nMax)
{
    char tmp[256];
    int i, j, nVal, nLen;

    if (isarray(str, &nLen))
    {
        if (nMax && nMax < nLen)
        {
            Warning(GetError(kErrInvalidArrayLen2), 1, nMax, nLen);
            return 0;
        }
    }

    i = 0;
    while (i < nMax && enumStr(i, str, tmp))
    {
        if (isIdKeyword(tmp, GetValType(kValFX), &nVal))
        {
            if (nVal >= kCudeFXEffectCallbackBase)
            {
                pOut[i] = CheckRange(tmp, nVal, kCudeFXEffectCallbackBase, kCudeFXEffectCallbackBase + countof(gCdudeCustomCallback) - 1);
            }
            else
            {
                pOut[i] = CheckRange(tmp, nVal, 1, 56) + kCdudeFXEffectBase; // it's ok to not do - 1
            }
        }
        else if (isIdKeyword(tmp, GetValType(kValGIB), &nVal))
        {
            pOut[i] = CheckRange(tmp, nVal, 1, 31) + kCdudeGIBEffectBase - 1;
        }
        else
        {
            Warning(GetError(kErrInvalidResultC), tmp, paramName); i++;
            continue;
        }

        // fill others
        for (j = i; j < nMax; j++) { pOut[j] = pOut[i]; }
        i++;
    }

    return i;
}

int CUSTOMDUDEV2_SETUP::ParseStatesToList(const char* str, TArray<int>& pOut)
{
    CUSTOMDUDE_WEAPON* pWeap;
    int i = 0, j, nVal, nPar;
    char tmp[256];
    const PARAM* pPar;

    while (enumStr(i++, str, tmp))
    {
        nPar = FindParam(tmp, gParamAnim);
        switch (nPar)
        {
            case kCdudeStateMove:
                for (j = kCdudeStateMoveBase; j < kCdudeStateMoveMax; j++)
                {
                    if ((pPar = FindParam(j, gParamAnim)) != nullptr)
                        pOut.AddUnique(pPar->id);
                }
                pOut.AddUnique(nPar);
                continue;
            case kCdudeStateDeath:
                for (j = kCdudeStateDeathBase; j < kCdudeStateDeathMax; j++)
                {
                    if ((pPar = FindParam(j, gParamAnim)) != nullptr)
                        pOut.AddUnique(pPar->id);
                }
                pOut.AddUnique(nPar);
                continue;
            case kCdudeStateAttack:
                for (j = 0; j < pDude->numWeapons; j++)
                {
                    pWeap = &pDude->weapons[j];
                    if (pWeap->group)
                    {
                        pOut.AddUnique(pWeap->stateID);
                        break;
                    }
                }
                pOut.AddUnique(nPar);
                continue;
            case kCdudeStateIdle:
                pOut.AddUnique(kCdudeStateGenIdle);
                pOut.AddUnique(nPar);
                continue;
            default:
                if (!rngok(nPar, kCdudeStateBase, kCdudeStateMax)) break;
                pOut.AddUnique(nPar);
                continue;
        }

        if (isIdKeyword(tmp, GetValType(kValWeapon), &nVal))
        {
            if (irngok(nVal, 1, kCdudeMaxWeapons))
            {
                j = pDude->numWeapons;
                while(--j >= 0)
                {
                    pWeap = &pDude->weapons[j];
                    if (pWeap->group == nVal)
                    {
                        pOut.AddUnique(pWeap->stateID);
                        break;
                    }
                }
                
                // weapon NA probably because of skill level
                // fill it with dummy value, otherwise it will
                // spawn effects anyway

                if (j < 0 && !pOut.Size())
                    pOut.AddUnique(kCdudeStateMax);
            }
            else Warning(GetError(kErrInvalidRange), tmp, 1, kCdudeMaxWeapons);
            continue;
        }

        Warning(GetError(kErrInvalidResultC), tmp, str);
    }

    return pOut.SSize();
}

bool CUSTOMDUDEV2_SETUP::ParseGibSetup(const char* str, CUSTOMDUDE_GIB* pGib)
{
    int i = 0, nPar, data[3];
    memset(data, 0, sizeof(data));
    pGib->Clear();

    if (!isempty(str))
    {
        while ((i = enumStr(i, str, key, val)) != 0)
        {
            nPar = FindParam(key, gParamGib);
            switch (nPar)
            {
                case kParGibType:
                    if (ParseIDs(val, kValGIB, data, countof(data)))
                    {
                        pGib->data1 = data[0];
                        pGib->data2 = data[1];
                        pGib->data3 = data[2];
                    }
                    break;
                case kParGibSoundID:
                    pGib->data4 = CheckValue(val, kValUfix, 0, 65535);
                    break;
                case kParGibFlags:
                    pGib->trFlags = ParseKeywords(val, gParamTriggerFlags);
                    break;
                case kParGibPhysics:
                    pGib->physics = ParseKeywords(val, gParamPhysics);
                    break;
                case kParGibForce:
                    pGib->force = CheckValue(val, kValBool, false);
                    break;
            }
        }

        pGib->available = true;
        return true;
    }

    pGib->available = false;
    return false;
}

void CUSTOMDUDEV2_SETUP::SetupEffect()
{
    CUSTOMDUDE_EFFECT* pEff;
    const char* pGroupText = pGroup->text;
    int i, nVal; int range[2];
    char tmp[64];

    /* ----------------------------------*/
    /* DEFAULT VALUES                    */
    /* ----------------------------------*/
    pDude->numEffects = 0;

    for (i = 0; i < kCdudeMaxEffectGroups; i++)
    {
        pEff = &pDude->effects[pDude->numEffects];
        pEff->Clear();

        snprintf(tmp, sizeof(tmp), pGroupText, i + 1);
        if (!DescriptGroupExist(tmp))
            continue;
        
        //pGroup->text = tmp;

        for (pParam = gParamEffect; pParam->text; pParam++)
        {
            pValue = DescriptGetValue(tmp, pParam->text);

            switch (pParam->id)
            {
                case kParEffectId:
                    if (pValue)
                    {
                        if (ParseEffectIDs(pValue, pParam->text, pEff->id, kCdudeMaxEffects) > 0)
                            break;
                    }
                    Warning(GetError(kErrReqParamNotFound), pParam->text);
                    continue;
                default:
                    if (pValue)
                    {
                        switch (pParam->id)
                        {
                            case kParEffectTimer:
                                range[0] = 1; range[1] = 4095;
                                switch (ParseRange(pValue, kValUfix, range))
                                {
                                    case 2:
                                        pEff->delay[0] = range[0];
                                        pEff->delay[1] = range[1];
                                        break;
                                    case 1:
                                        pEff->delay[0] = range[0];
                                        pEff->delay[1] = 0;
                                        break;
                                }
                                break;
                            case kParEffectAngle:
                                nVal = kAng360 - 1;
                                pEff->angle = DAngle::fromBuild(CheckValue(pValue, kValFix, -nVal, nVal, 0));
                                break;
                            case kParEffectMedium:
                                if ((nVal = ParseMedium(pValue)) >= 0)
                                {
                                    pEff->medium = nVal;
                                    break;
                                }
                                Warning(GetError(kErrInvalidResultC), pValue, pParam->text);
                                break;
                            case kParEffectAiState:     ParseStatesToList(pValue, pEff->pStates);                       break;
                            case kParEffectAnimFrame:   ParseIDs(pValue, kValUfix, pEff->pFrames);                      break;
                            case kParEffectAnimID:      ParseIDs(pValue, kValUfix, pEff->pAnims);                       break;
                            case kParEffectOffset:      ParseOffsets(pValue, pEff->offset);                             break;
                            case kParEffectFx2Gib:      ParseGibSetup(pValue, &pEff->spr2gib);                          break;
                            case kParEffectAppearance:  ParseAppearance(pValue, &pEff->appearance);                     break;
                            case kParEffectPosture:     pEff->posture       = ParsePosture(pValue);                     break;
                            case kParEffectVelocity:    pEff->_velocity      = CheckValue(pValue, kValUfix, 0) / 64.;   break; //The factors here are rather arbitrary...
                            case kParEffectSlope:       pEff->_velocitySlope = CheckValue(pValue, kValFix, 0) / 16.;    break;
                            case kParEffectRemTime:     pEff->liveTime      = CheckValue(pValue, kValFix, -1, 65535);   break;
                            case kParEffectAllUnique:   pEff->allUnique     = CheckValue(pValue, kValBool, 0);          break;
                            case kParEffectSrcVel:      pEff->srcVelocity   = CheckValue(pValue, kValBool, 1);          break;
                            case kParEffectChance:
                                nVal = CheckValue(pValue, kValPerc, 0, 100, 100);
                                pEff->chance = perc2val(nVal, 0x10000);
                                break;
                        }
                    }
                    break;
            }
        }

        pDude->numEffects++;

    }
}

void CUSTOMDUDEV2_SETUP::SetupMovePattern(void)
{
    int nVal;
    pDude->fallHeight = INT32_MAX;

    pParam = gParamMovePat;
    while (pParam->id != kParamMax)
    {
        pValue = DescriptGetValue(pGroup->text, pParam->text);
        if (pValue)
        {
            switch (pParam->id)
            {
                case kParMoveFallHeight:
                    nVal = CheckValue(pValue, kValUfix, INT32_MAX);
                    if (nVal != INT32_MAX) pDude->fallHeight = nVal << 4;
                    break;
            }
        }

        pParam++;
    }

}

bool CUSTOMDUDEV2_SETUP::ParseWeaponBasicInfo(const char* str, CUSTOMDUDE_WEAPON* pWeap)
{
    int i, nMin, nMax, nID;
    bool isNum = isufix(str);
    const WEAPINFO* pInfo;

    if (isempty(str))
    {
        isNum = true;
        nID = 0;
    }
    else if (isNum)
    {
        nID = CheckValue(str, kValUfix, 0);
    }

    for (i = 0; i < countof(gCdudeWeaponInfo); i++)
    {
        pInfo = &gCdudeWeaponInfo[i];
        if (isNum)
        {
            // classic style
            if (nID != 0)
            {
                if (!rngok(nID, pInfo->range[0], pInfo->range[1]))
                    continue;
            }
            
            pWeap->id = nID;
        }
        else if (isIdKeyword(str, pInfo->keyword, &nID))
        {
            nMin        = 1;
            nMax        = pInfo->range[1] - pInfo->range[0];
            nID         = CheckRange(str, nID, nMin, nMax) - 1;
            pWeap->id   = pInfo->range[0] + nID;
        }
        else if (isNone(str))
        {
            pWeap->id = 0;
        }
        else
        {
            continue;
        }

        pWeap->clipMask = pInfo->clipMask;
        pWeap->type     = pInfo->type;
        return true;
    }

    return false;
}

bool CUSTOMDUDEV2_SETUP::ParseSkill(const char* str)
{
    int range[2] = {1, 5};
    int nSkill = gGameOptions.nDifficulty + 1;
    switch (ParseRange(str, kValUfix, range))
    {
        case 1:
            if (nSkill != range[0]) return false;
            break;
        case 2:
            if (!rngok(nSkill, range[0], range[1])) return false;
            break;
        default:
            return false;
    }

    return true;
}

bool CUSTOMDUDEV2_SETUP::ParseDropItem(const char* str, CUSTOMDUDE_DROPITEM::itemdesc& out)
{
    int nPar, nVal, i = 0;
    uint8_t nItem = 0;
    uint8_t nPerc = 100;

    out.cls = nullptr;
    out.perc = 0;

    if (isarray(str))
    {
        while ((i = enumStr(i, str, key, val)) != 0)
        {
            nPar = FindParam(key, gParamDropItem);
            switch (nPar)
            {
                case kParDropItemSkill:
                    if (!ParseSkill(val))
                        return false;
                    break;
                case kParDropItemChance:
                    nPerc = CheckValue(val, kValPerc, 0, 100, 100);
                    break;
                case kParDropItemType:
                    nItem = CheckValue(val, kValUfix, kItemWeaponBase, kItemMax - 1, 0);
                    break;
            }
        }

        if (nItem)
        {
            out.cls = GetSpawnType(nItem);
            out.perc = nPerc;
            return true;
        }
    }
    else if (!isempty(str))
    {
        nVal = CheckValue(str, kValUfix, kItemWeaponBase, kItemMax - 1, 0);
        if (nVal)
        {
            out.cls = GetSpawnType(nVal);
            out.perc = 100;
            return true;
        }
    }

    return false;
}

void CUSTOMDUDEV2_SETUP::SetupDropItem(void)
{
    int nVal;
    int i, c; char tmp[64];
    CUSTOMDUDE_DROPITEM* pDrop = &pDude->dropItem;
    const PARAM* pItem = FindParam(kParDropItem, gParamDropItem);
    
    // global params first
    pParam = gParamDropItem;
    while (pParam->id != kParamMax)
    {
        pValue = DescriptGetValue(pGroup->text, pParam->text);
        if (pValue)
        {
            switch (pParam->id)
            {
                case kParDropItemSprChance:
                    nVal = CheckValue(pValue, kValPerc, 0, 100, 100);
                    pDrop->sprDropItemChance = perc2val(nVal, 0x10000);
                    break;
            }
        }

        pParam++;
    }
        
    // ItemN params then
    for (i = 0, c = 0; i < kCdudeMaxDropItems; i++)
    {
        snprintf(tmp, sizeof(tmp), pItem->text, i + 1);

        pValue = DescriptGetValue(pGroup->text, tmp);
        if (pValue)
        {
            if (ParseDropItem(pValue, pDrop->items[c]))
                c++;
        }
    }
}

void CUSTOMDUDEV2_SETUP::SetupWeapons(void)
{
    int nVal, i, t = -1;
    const char* pGroupText = pGroup->text;
    char tmp[64]; int range[2];

    CUSTOMDUDE_WEAPON* pWeap;
    AISTATE* pState;

    /* ----------------------------------*/
    /* DEFAULT VALUES                    */
    /* ----------------------------------*/
    pDude->numWeapons = 0;

    for (i = 0; i < kCdudeMaxWeapons; i++)
    {
        pWeap = &pDude->weapons[pDude->numWeapons];
        pWeap->Clear();

        snprintf(tmp, sizeof(tmp), pGroupText, i + 1);
        if (!DescriptGroupExist(tmp))
            continue;

        // search for skill settings first
        // ----------------------------------------------
        pParam = FindParam(kParWeaponSkill, gParamWeapon);
        if (pParam && (pValue = DescriptGetValue(tmp, pParam->text)) != nullptr)
        {
            if (!ParseSkill(pValue))
                continue;
        }

        pWeap->group = i + 1;
        pDude->numWeapons++;

        for (pParam = gParamWeapon; pParam->id != kParamMax; pParam++)
        {
            pValue = DescriptGetValue(tmp, pParam->text);
            switch (pParam->id)
            {
                case kParWeaponAttackAng:
                case kParWeaponDist:
                case kParWeaponPickChance:
                    switch (pParam->id)
                    {
                        case kParWeaponAttackAng:
                            pWeap->angle = DAngle::fromBuild(CheckValue(pValue, kValUfix, 0, kAng360, kAng15));
                            break;
                        case kParWeaponDist:
                            range[0] = 0; range[1] = 0x10000;
                            switch (ParseRange(pValue, kValUfix, range))
                            {
                                case 2:
                                    pWeap->distRange[0] = range[0] * 0.5;  // why this factor? It was << 3, although distance is << 4!
                                    pWeap->distRange[1] = range[1] * 0.5;
                                    break;
                                default:
                                    Warning(GetError(kErrInvalidValType), pValue, GetValType(kValArrC));
                                    pWeap->distRange[0] = 0;
                                    pWeap->distRange[1] = 32767 * inttoworld;
                                    break;
                            }
                            break;
                        case kParWeaponPickChance:
                            nVal = CheckValue(pValue, kValPerc, 0, 100, 100);
                            pWeap->pickChance = perc2val(nVal, 0x10000);
                            break;
                    }
                    break;
                case kParWeaponId:
                    if (ParseWeaponBasicInfo(pValue, pWeap)) break;
                    Warning(GetError(kErrInvalidResultC), pValue, pParam->text);
                    break;
                case kParWeaponCooldown:
                    if (pValue)
                    {
                        switch (ParseIDs(pValue, kValUfix, range, 2))
                        {
                            case 2:
                                pWeap->cooldown.delay           = CheckRange(pParam->text, range[0], 1, 32767);
                                pWeap->cooldown.totalUseCount   = CheckRange(pParam->text, range[1], 1, 16384);
                                break;
                            case 1:
                                pWeap->cooldown.delay           = CheckRange(pParam->text, range[0], 1, 32767);
                                break;
                        }
                    }
                    break;
                case kParWeaponMedium:
                    if (pValue)
                    {
                        if ((nVal = ParseMedium(pValue)) >= 0)
                        {
                            pWeap->medium = nVal;
                            break;
                        }
                        
                        Warning(GetError(kErrInvalidResultC), pValue, pParam->text);
                    }
                    break;
                case kParWeaponDudeHealth:
                case kParWeaponTargetHealth:
                    if (pValue)
                    {
                        range[0] = 0; range[1] = 200;
                        if (ParseRange(pValue, kValPerc, range) == 2)
                        {
                            switch (pParam->id)
                            {
                                case kParWeaponDudeHealth:
                                    pWeap->dudeHpRange[0] = range[0];
                                    pWeap->dudeHpRange[1] = range[1];
                                    break;
                                default:
                                    pWeap->targHpRange[0] = range[0];
                                    pWeap->targHpRange[1] = range[1];
                                    break;
                            }

                            break;
                        }

                        Warning(GetError(kErrInvalidValType), pValue, GetValType(kValArrC));
                    }
                    break;
                case kParWeaponPosture:
                    pWeap->posture = ParsePosture(pValue);
                    break;
                case kParWeaponStyle:
                case kParWeaponShotSetup:
                case kParWeaponAttackSetup:
                    if (pValue)
                    {
                        if (isarray(pValue))
                        {
                            switch (pParam->id)
                            {
                                case kParWeaponStyle:           ParseWeaponStyle(pValue, pWeap);    break;
                                case kParWeaponShotSetup:       ParseShotSetup(pValue, pWeap);      break;
                                case kParWeaponAttackSetup:     ParseAttackSetup(pValue, pWeap);    break;
                            }

                            break;
                        }

                        Warning(GetError(kErrInvalidValType), pValue, GetValType(kValArrA));
                    }
                    break;
                case kParWeaponAttackAnim:
                    pWeap->stateID = kCdudeStateAttackBase + pDude->numWeapons - 1;
                    pWeap->nextStateID = pWeap->stateID;

                    pState = pDude->states[pWeap->stateID];
                    SetupAnimation(pState, true);
                    for (t = 0; t < kCdudePostureMax; t++)
                    {
                        if (!helperSeqTriggerExists(pState->seqId))
                        {
                            Seq* pSeq = getSequence(pState->seqId);
                            if (pSeq)
                                pSeq->frames[pSeq->nFrames - 1].trigger = 1;
                        }

                        pState++;
                    }
                    break;
                case kParWeaponAmmo:
                case kParWeaponDisp:
                case kParWeaponAkimbo:
                    if (pValue)
                    {
                        switch (pParam->id)
                        {
                            case kParWeaponAmmo:
                                nVal = CheckValue(pValue, kValUfix, 0, 32767);
                                pWeap->ammo.SetTotal(nVal); pWeap->ammo.SetFull();
                                break;
                            case kParWeaponDisp:
                                range[0] = 0; range[1] = 1024;
                                switch (ParseRange(pValue, kValPerc, range, kCdudeMaxDispersion))
                                {
                                    case 2:
                                        pWeap->dispersion[0] = range[0];
                                        pWeap->dispersion[1] = range[1];
                                        break;
                                    case 1:
                                        pWeap->dispersion[0] = range[0];
                                        pWeap->dispersion[1] = range[0] >> 1;
                                        break;
                                    default:
                                        Warning(GetError(kErrInvalidResultC), pValue, pParam->text);
                                        break;
                                }
                                break;
                            case kParWeaponAkimbo:
                                pWeap->sharedId = CheckValue(pValue, kValUfix, 0, kCdudeMaxWeapons);
                                break;
                        }
                    }
                    break;
                case kParWeaponShotSnd:
                    if (ParseSound(pValue, &pWeap->sound)) break;
                    else WeaponSoundSetDefault(pWeap);
                    break;
                case kParWeaponShotAppearance:
                    ParseAppearance(pValue, &pWeap->shot.appearance);
                    break;
            }
        }
    }

    if (pDude->numWeapons > 1) // weapon with lowest pickChance in top
        qsort(pDude->weapons, pDude->numWeapons, sizeof(pDude->weapons[0]), (int(*)(const void*, const void*))qsSortWeapons);
}



bool CUSTOMDUDEV2_SETUP::ParseVelocity(const char* str, CUSTOMDUDE_VELOCITY* pVelocity)
{
    int nMod = pDude->pSpr->xspr.busyTime;
    int i, nPar, nVal;
        
    if (isarray(str))
    {
        i = 0, nVal = -1;
        while (enumStr(i++, str, key, val))
        {
            nPar = FindParam(key, gParamVelocity);
            switch (nPar)
            {
                case kParVelocityDodge:
                case kParVelocityForward:
                    nVal = CheckValue(val, kValUfix, 0, kCdudeMaxVelocity, -1);
                    if (nVal != -1)
                    {
                        nVal <<= 2;
                        if (nMod && nPar == kParVelocityForward) // allow to mod forward velocity by busyTime
                            nVal = ClipHigh((nVal / 3) + (2500 * nMod), kCdudeMaxVelocity);
                        
                        break;
                    }
                    continue;
                case kParVelocityTurn:
                    nVal = CheckValue(val, kValUfix, 0, 100, -1);
                    if (nVal != -1)
                    {
                        nVal <<= 2;
                        break;
                    }
                    continue;
            }

            pVelocity->Set(nVal, nPar);
        }

        return true;
    }
    else if (!isempty(str))
    {
        nVal = CheckValue(str, kValUfix, 0, kCdudeMaxVelocity, -1);
        if (nVal != -1)
        {
            nVal <<= 2;
            if (nMod)
                nVal = ClipHigh((nVal / 3) + (2500 * nMod), kCdudeMaxVelocity); // allow to mod forward velocity by busyTime

            pVelocity->Set(nVal, kParVelocityForward);
            return true;
        }
    }

    return false;
}

bool CUSTOMDUDEV2_SETUP::ParseAppearance(const char* str, APPEARANCE* pAppear)
{
    int i = 0, nPar, range[2];
    pAppear->Clear();

    if (!isempty(str))
    {
        while ((i = enumStr(i, str, key, val)) != 0)
        {
            nPar = FindParam(key, gParamAppearance);
            switch (nPar)
            {
                case kAppearClb:
                    pAppear->clb = CheckValue(val, kValUfix, 0, countof(gCdudeCustomCallback));
                    break;
                case kAppearSeq:
                    pAppear->seq = CheckValue(val, kValUfix, 0, 65535);
                    break;
                case kAppearScale:
                    range[0] = 0; range[1] = 1024;
                    switch (ParseRange(val, kValPerc, range, kCdudeDefaultAnimScale))
                    {
                        case 2:
                            pAppear->scl[0] = range[0];
                            pAppear->scl[1] = range[1];
                            break;
                        case 1:
                            pAppear->scl[0] = range[0];
                            pAppear->scl[1] = 0;
                            break;
                    }
                    break;
                case kAppearSnd:
                    pAppear->soundAvailable = ParseSound(val, &pAppear->sound);
                    pAppear->sound.once     = true;
                    //pAppear->sound.ai       = true;
                    break;
                case kAppearPic:
                    pAppear->pic = tileGetTextureID(CheckValue(val, kValUfix, 1, kMaxTiles-1, 0));
                    break;
                case kAppearShade:
                    pAppear->shd = CheckValue(val, kValFix, -128, 64, 127);
                    break;
                case kAppearPal:
                    pAppear->pal = CheckValue(val, kValUfix, 0, 255);
                    break;
                case kAppearSize:
                    switch (ParseIDs(val, kValUfix, range, 2))
                    {
                        case 2:
                            pAppear->xrp = CheckRange(val, range[0], 0, 255);
                            pAppear->yrp = CheckRange(val, range[1], 0, 255);
                            break;
                        default:
                            Warning(GetError(kErrInvalidValType), val, GetValType(kValArrC));
                            break;
                    }
                    break;
                case kAppearOffs1:
                    switch (ParseIDs(val, kValFix, range, 2))
                    {
                        case 2:
                            pAppear->xof = CheckRange(val, range[0], -128, 127);
                            pAppear->yof = CheckRange(val, range[1], -128, 127);
                            break;
                        default:
                            Warning(GetError(kErrInvalidValType), val, GetValType(kValArrC));
                            break;
                    }
                    break;
            }
        }

        pAppear->available = true;
        return true;
    }

    pAppear->available = false;
    return false;
}

bool CUSTOMDUDEV2_SETUP::ParseSound(const char* str, CUSTOMDUDE_SOUND* pSound)
{
    int i = 0, j, nMedium = pSound->medium, nVolume = pSound->volume;
    int t = kCdudeMaxSounds, nVal, nLen;
    char tmp[256];

    if ((nLen = CheckArray(str, 0, t + 2, 0)) > 0)
    {
        // first 1 - 3 elements supposed to be ufix
        while (i < t && enumStr(i, str, tmp) && isufix(tmp))
        {
            nVal = CheckValue(tmp, kValUfix, 0, 65535);
            for (j = i; j < t; j++) pSound->id[j] = soundEngine->FindSoundByResID(nVal);
            i++;
        }

        // at least 1 sound id
        if (!i)
        {
            Warning(GetError(kErrInvalidParam), tmp);
            t = 0;
        }
        else if (t < nLen) // continue parsing
            t += 2;

        // next is the sound volume % and/or medium
        while (i < t && enumStr(i, str, tmp))
        {
            if (isperc(tmp))
            {
                if (nLen - i <= 2)
                {
                    nVolume = CheckValue(tmp, kValPerc, 0, 1023, 100);
                }
                else
                {
                    Warning(GetError(kErrInvaliValuePos), tmp);
                    break;
                }
            }
            else if ((nMedium = ParseMedium(tmp)) >= 0)
            {
                if (nLen - i != 1)
                {
                    Warning(GetError(kErrInvaliValuePos), tmp);
                    break;
                }
            }
            else
            {
                Warning(GetError(kErrInvalidParam), tmp);
                break;
            }

            i++;
        }
        
        if (t)
        {
            pSound->volume = perc2val(nVolume, 255);
            pSound->medium = nMedium;
            return true;
        }
    }
    else if (!isempty(str))
    {
        nVal = CheckValue(str, kValUfix, 0, 65535);
        pSound->medium = kParMediumAny;
        SoundFill(pSound, nVal);
        return true;
    }

    return false;
}

bool CUSTOMDUDEV2_SETUP::ParseAnimation(const char* str, AISTATE* pState, bool asPosture)
{
    int i, j, nPar, nLen;
    int nVal;

    if ((nLen = CheckArray(str, 0, kCdudePostureMax, 0)) > 0)
    {
        for (i = 0; i < nLen; i++)
        {
            if (asPosture)
            {
                if (enumStr(i, str, key, val))
                {
                    nPar = FindParam(key, gParamPosture);
                    if (rngok(nPar, kCdudePosture, kCdudePostureMax))
                        pState[nPar].seqId = CheckValue(val, kValUfix, 0, 65535);

                    continue;
                }
            }
            else if (enumStr(i, str, val))
            {
                nVal = CheckValue(val, kValUfix, 0, 65535);
                
                pState[i].seqId = nVal;
                for (j = i; j < kCdudePostureMax; j++)
                    pState[j].seqId = nVal;

                continue;
            }

            break;
        }
        
        return (i > 0);
    }
    else if (!isempty(str))
    {
        i = (asPosture) ? kCdudePostureL : 0;
        pState[i].seqId = CheckValue(str, kValUfix, 0, 65535);
        if (!asPosture)
            AnimationFill(pState, pState[i].seqId);

        return true;
    }

    return false;
}

int CUSTOMDUDEV2_SETUP::ParseRange(const char* str, int nValType, int out[2], int nBaseVal)
{
    int nLen, nVal;
    int nMin = out[0];
    int nMax = out[1];
    bool chkrng = (nMin && nMax);
    char tmp[256];
    int i = 0;

    if (!isempty(str))
    {
        nLen = CheckArray(str, 0, 2);
        while (i < nLen && enumStr(i, str, tmp))
        {
            if (chkrng)
            {
                nVal = CheckValue(tmp, nValType, nMin, nMax);
            }
            else
            {
                nVal = CheckValue(tmp, nValType, 0);
            }

            switch (nValType)
            {
                case kValPerc:
                    if (nBaseVal) nVal = perc2val(nVal, nBaseVal);
                    break;
            }

            out[i++] = nVal;
        }

        return i;
    }

    return 0;
}

int CUSTOMDUDEV2_SETUP::ParseMedium(const char* str)
{
    switch (FindParam(str, gParamMedium))
    {
        case kParMediumLand:   return 0x01;
        case kParMediumWater:  return 0x02;
        case kParMediumAny:    return 0x00;
        default:               return   -1;
    }
}

int CUSTOMDUDEV2_SETUP::CheckArray(const char* str, int nMin, int nMax, int nDefault)
{
    int nLen;
    if (isarray(str, &nLen))
    {
        if (nMax == 0 && nMin > 0 && nLen < nMin)
        {
            Warning(GetError(kErrInvalidArrayLen1), nMin, nLen);
            return nDefault;
        }

        if ((nMin > 0 && nLen < nMin) || (nMax > 0 && nLen > nMax))
        {
            Warning(GetError(kErrInvalidArrayLen2), nMin, nMax, nLen);
            return nDefault;
        }

        return nLen;
    }

    return nDefault;
}

bool CUSTOMDUDEV2_SETUP::ParseOffsets(const char* str, DVector3& pOut)
{
    int i, nLen; char tmp[256];
    pOut.Zero();
    nLen = CheckArray(str, 3, 0, 0);
    double nVal;

    i = 0;
    while (i < nLen && enumStr(i, str, tmp))
    {
        nVal = CheckValue(tmp, kValFix, -16383, 16383, 0) * inttoworld;
        switch (i)
        {
        case 0: pOut.X = nVal;       break;
        case 1: pOut.Y = nVal;       break;
        case 2: pOut.Z = nVal;       break; // the parser uses the same format as for xy.
        }

        i++;
    }

    return i;
}

bool CUSTOMDUDEV2_SETUP::ParseShotSetup(const char* str, CUSTOMDUDE_WEAPON* pWeap)
{
    int i, nPar, nVal, nVel;
    if (isarray(str))
    {
        i = 0;
        while ((i = enumStr(i, str, key, val)) != 0)
        {
            nPar = FindParam(key, gWeaponShotSetup);
            switch (nPar)
            {
                case kParWeaponShotOffs:
                    ParseOffsets(val, pWeap->shot.offset);
                    break;
                case kParWeaponShotVel:
                    nVel = CheckValue(val, kValUfix, INT32_MAX);
                    if (nVel != INT32_MAX)
                        pWeap->shot._velocity = nVel / 64.;
                    else 
                        pWeap->shot._velocity = FLT_MAX;
                    break;
                case kParWeaponShotSlope:
                    pWeap->shot.slope = CheckValue(val, kValFix, INT32_MAX);
                    if (pWeap->shot.slope != INT32_MAX)
                        pWeap->shot.slope <<= 4;
                    break;
                case kParWeaponShotFollow:
                    nVal = CheckValue(val, kValUfix, 0, kAng360);
                    pWeap->shot.targetFollow = DAngle::fromBuild(ClipHigh(nVal, kAng360 - 1));
                    break;
                case kParWeaponShotClipdist:
                    pWeap->shot.clipdist = CheckValue(val, kValUfix, 0, 255, 32);
                    break;
                case kParWeaponShotImpact:
                    pWeap->shot.impact = CheckValue(val, kValBool, false);
                    break;
                case kParWeaponShotRemTimer:
                    pWeap->shot.remTime = CheckValue(val, kValUfix, 0, 4095);
                    break;
            }
        }

        return true;
    }

    return false;
}

bool CUSTOMDUDEV2_SETUP::ParseWeaponStyle(const char* str, CUSTOMDUDE_WEAPON* pWeap)
{
    memset(&pWeap->style, 0, sizeof(pWeap->style));
    int nPar, i = 0;

    i = 0;
    while ((i = enumStr(i, str, key, val)) != 0)
    {
        nPar = FindParam(key, gParamWeaponStyle);
        switch (nPar)
        {
            case kParWeaponStyleOffset:
                ParseOffsets(val, pWeap->style.offset);
                break;
            case kParWeaponStyleAngle:
                pWeap->style.angle = DAngle::fromBuild(CheckValue(val, kValUfix, 0, kAng360));
                break;
        }
    }

    pWeap->style.available = true;
    return true;
}

bool CUSTOMDUDEV2_SETUP::ParseAttackSetup(const char* str, CUSTOMDUDE_WEAPON* pWeap)
{
    int i = 0, j, nPar, nVal;
    while ((i = enumStr(i, str, key, val)) != 0)
    {
        nPar = FindParam(key, gParamAttack);
        switch (nPar)
        {
            case kParAttackTime:
                nVal = CheckValue(val, kValUfix, 0, 32767);
                for (j = 0; j < kCdudePostureMax; j++)
                    pDude->states[pWeap->stateID][j].stateTicks = nVal;
                break;
            case kParAttackInterrupt:
            case kParAttackTurn2Target:
                nVal = CheckValue(val, kValBool, false);
                switch (nPar)
                {
                    case kParAttackInterrupt:     pWeap->interruptable = nVal;    break;
                    case kParAttackTurn2Target:   pWeap->turnToTarget  = nVal;    break;
                }
                break;
            case kParAttackNumShots:
                pWeap->numshots = CheckValue(val, kValUfix, 1, 63);
                break;
            case kParAttackInertia:
                if (CheckValue(val, kValBool, false) == 1)
                {
                    // remove reset velocity function (keeps moving with inertia)
                    for (j = 0; j < kCdudePostureMax; j++)
                        pDude->states[pWeap->stateID][j].enterFunc = nullptr;
                }
                break;
        }
    }

    return true;
}


int CUSTOMDUDEV2_SETUP::ParsePosture(const char* str)
{
    int nRetn = 0;
    int i, nVal;

    i = 0;
    while (i < kCdudePostureMax && enumStr(i++, str, key))
    {
        nVal = FindParam(key, gParamPosture);
        switch (nVal)
        {
            case kCdudePostureL:    nRetn |= 0x01;   break;
            case kCdudePostureC:    nRetn |= 0x02;   break;
            case kCdudePostureW:    nRetn |= 0x04;   break;
        }
    }

    return nRetn;
}

bool CUSTOMDUDEV2_SETUP::ParseOnEventDmg(const char* str, int* pOut, int nLen)
{
    int i;
    if (isarray(str))
    {
        i = 0;
        while (i < nLen && enumStr(i, str, key))
        {
            switch (i)
            {
                case 0: pOut[i] = CheckValue(key, kValUfix, 0, 2048) << 4;                   break;
                case 1: pOut[i] = perc2val(CheckValue(key, kValPerc,  0, 100), 0x10000);     break;
                case 2: pOut[i] = CheckValue(key, kValUfix, 0, 2048);                        break;
            }

            i++;
        }

        return true;
    }
    
    Warning(GetError(kErrInvalidValType), str, GetValType(kValArrC));
    return false;
}

void CUSTOMDUDEV2_SETUP::Setup(void)
{
    assert(pDude != nullptr);
    assert(pIni != nullptr);

    auto pSpr = pDude->pSpr;

    pGroup = gParGroup;
    while (pGroup->id != kParamMax)
    {
        switch (pGroup->id)
        {
            case kParGroupGeneral:       SetupGeneral();       break;
            case kParGroupVelocity:      SetupVelocity();      break;
            case kParGroupAnimation:     SetupAnimation();     break;
            case kParGroupSound:         SetupSound();         break;
            case kParGroupDamage:        SetupDamage();        break;
            case kParGroupRecoil:        SetupRecoil();        break;
            case kParGroupDodge:         SetupDodge();         break;
            case kParGroupKnockout:      SetupKnockout();      break;
            case kParGroupWeapon:        SetupWeapons();       break;
            case kParGroupFXEffect:        SetupEffect();        break;
            case kParGroupMovePat:       SetupMovePattern();   break;
            case kParGroupDropItem:      SetupDropItem();      break;
        }

        pGroup++;
    }

    if (pSpr->xspr.data2 & kCdudeStatusAwaked)
    {
        AnimationFill(pDude->states[kCdudeStateSleep], 0);
        pDude->StatusSet(kCdudeStatusAwaked);
    }

    if (pSpr->xspr.data2 & kCdudeStatusForceCrouch)
        pDude->StatusSet(kCdudeStatusForceCrouch);
}


/*************************************************************************************************/


void CUSTOMDUDEV1_SETUP::DamageScaleToWeapon(CUSTOMDUDE_WEAPON* pWeapon)
{
    CUSTOMDUDE_DAMAGE* pDmg = &pDude->damage;
    
    switch (pWeapon->type)
    {
        case kCdudeWeaponKamikaze:
            pDmg->Set(1024, kDmgBurn);
            pDmg->Set(1024, kDmgExplode);
            pDmg->Set(1024, kDmgElectric);
            break;
        case kCdudeWeaponMissile:
        case kCdudeWeaponThrow:
            switch (pWeapon->id)
            {
                case kMissileButcherKnife:
                    pDmg->Set(100, kDmgBullet);
                    pDmg->Set(32, kDmgSpirit);
                    break;
                case kMissileLifeLeechAltNormal:
                case kMissileLifeLeechAltSmall:
                case kMissileArcGargoyle:
                    pDmg->Dec(32, kDmgSpirit);
                    pDmg->Set(52, kDmgElectric);
                    break;
                case kMissileFlareRegular:
                case kMissileFlareAlt:
                case kMissileFlameSpray:
                case kMissileFlameHound:
                case kThingArmedSpray:
                case kThingPodFireBall:
                case kThingNapalmBall:
                    pDmg->Set(32, kDmgBurn);
                    break;
                case kThingDroppedLifeLeech:
                case kModernThingEnemyLifeLeech:
                    pDmg->Set(32, kDmgSpirit);
                    break;
                case kMissileFireball:
                case kMissileFireballNapalm:
                case kMissileFireballCerberus:
                case kMissileFireballTchernobog:
                    pDmg->Set(50, kDmgBurn);
                    pDmg->Dec(32, kDmgExplode);
                    pDmg->Set(65, kDmgFall);
                    break;
                case kThingTNTBarrel:
                case kThingArmedProxBomb:
                case kThingArmedRemoteBomb:
                case kThingArmedTNTBundle:
                case kThingArmedTNTStick:
                case kModernThingTNTProx:
                    pDmg->Dec(32, kDmgBurn);
                    pDmg->Dec(32, kDmgExplode);
                    pDmg->Set(65, kDmgFall);
                    break;
                case kMissileTeslaAlt:
                case kMissileTeslaRegular:
                    pDmg->Set(32, kDmgElectric);
                    break;
            }
            break;
    }
}


void CUSTOMDUDEV1_SETUP::DamageScaleToSurface(int nSurface)
{
    CUSTOMDUDE_DAMAGE* pDmg = &pDude->damage;

    switch (nSurface)
    {
        case kSurfStone:
            pDmg->Set(0, kDmgFall);       pDmg->Dec(200, kDmgBullet);
            pDmg->Dec(100, kDmgBurn);     pDmg->Dec(80, kDmgExplode);
            pDmg->Inc(30, kDmgChoke);     pDmg->Inc(20, kDmgElectric);
            break;
        case kSurfMetal:
            pDmg->Dec(16, kDmgFall);      pDmg->Dec(128, kDmgBullet);
            pDmg->Dec(90, kDmgBurn);      pDmg->Dec(55, kDmgExplode);
            pDmg->Inc(20, kDmgChoke);     pDmg->Inc(30, kDmgElectric);
            break;
        case kSurfWood:
            pDmg->Dec(10, kDmgBullet);    pDmg->Inc(50, kDmgBurn);
            pDmg->Inc(40, kDmgExplode);   pDmg->Inc(10, kDmgChoke);
            pDmg->Dec(60, kDmgElectric);
            break;
        case kSurfWater:
        case kSurfDirt:
        case kSurfClay:
        case kSurfGoo:
            pDmg->Set(8, kDmgFall);       pDmg->Dec(20, kDmgBullet);
            pDmg->Dec(200, kDmgBurn);     pDmg->Dec(60, kDmgExplode);
            pDmg->Set(0, kDmgChoke);      pDmg->Inc(40, kDmgElectric);
            break;
        case kSurfSnow:
        case kSurfIce:
            pDmg->Set(8, kDmgFall);       pDmg->Dec(20, kDmgBullet);
            pDmg->Dec(100, kDmgBurn);     pDmg->Dec(50, kDmgExplode);
            pDmg->Set(0, kDmgChoke);      pDmg->Inc(40, kDmgElectric);
            break;
        case kSurfLeaves:
        case kSurfPlant:
        case kSurfCloth:
            pDmg->Set(0, kDmgFall);       pDmg->Dec(10, kDmgBullet);
            pDmg->Inc(70, kDmgBurn);      pDmg->Inc(50, kDmgExplode);
            break;
        case kSurfLava:
            pDmg->Set(0, kDmgBurn);
            pDmg->Set(0, kDmgExplode);
            pDmg->Inc(30, kDmgChoke);
            break;
    }
}

void CUSTOMDUDEV1_SETUP::WeaponMeleeSet(CUSTOMDUDE_WEAPON* pWeapon)
{
    bool availStatus = pWeapon->available;
    pWeapon->Clear();
    
    pWeapon->type           = kCdudeWeaponHitscan;
    pWeapon->id             = kVectorGenDudePunch;
    pWeapon->stateID        = kCdudeStateAttackBase + 2;
    pWeapon->angle          = DAngle90;
    pWeapon->clipMask       = CLIPMASK1;
    pWeapon->available      = availStatus;

    WeaponRangeSet(pWeapon, 0, 512);

    AISTATE* pState = pDude->states[pWeapon->stateID];
    for (int i = 0; i < kCdudePostureMax; i++)
    {
        if (!helperSeqTriggerExists(pState->seqId))
        {
            Seq* pSeq = getSequence(pState->seqId);
            if (pSeq)
                pSeq->frames[pSeq->nFrames - 1].trigger = 1;
        }

        pState++;
    }
}

void CUSTOMDUDEV1_SETUP::WeaponConvert(int nWeaponID)
{
    CUSTOMDUDE_WEAPON* pW1 = &pDude->weapons[0];
    CUSTOMDUDE_WEAPON* pW2 = &pDude->weapons[1];
    bool availStatus = pW1->available;

    pW1->Clear();
    if (rngok(nWeaponID, 1, kVectorMax))                                       pW1->type = kCdudeWeaponHitscan,     pW1->clipMask = CLIPMASK1;
    else if (rngok(nWeaponID, kDudeBase, kDudeMax))                            pW1->type = kCdudeWeaponSummon,      pW1->clipMask = CLIPMASK0;
    else if (rngok(nWeaponID, kMissileBase, kMissileMax))                      pW1->type = kCdudeWeaponMissile,     pW1->clipMask = CLIPMASK0;
    else if (rngok(nWeaponID, kThingBase, kThingMax))                          pW1->type = kCdudeWeaponThrow,       pW1->clipMask = CLIPMASK0;
    else if (rngok(nWeaponID, kTrapExploder, kTrapExploder + kExplodeMax))     pW1->type = kCdudeWeaponKamikaze,    pW1->clipMask = CLIPMASK0;
    else                                                                       pW1->type = kCdudeWeaponNone;

    pW1->id                         = nWeaponID;
    pW1->available                  = availStatus;
    pDude->numWeapons               = 1;

    pW1->ammo.SetTotal(0);  pW1->ammo.SetFull();
    WeaponDispersionSetDefault(pW1);

    if (pW1->type == kCdudeWeaponNone)
    {
        WeaponRangeSet(pW1, 0, kCdudeV1MaxAttackDist);
        pW1->angle = DAngle360;
    }
    else if (pW1->type == kCdudeWeaponThrow)
    {
        pW1->angle      = DAngle15;

        switch (pW1->id)
        {
            case kModernThingEnemyLifeLeech:
            case kThingDroppedLifeLeech:
                pW1->shot.slope = -5000;
                pW1->ammo.SetTotal(1);
                pW1->ammo.SetFull();
                [[fallthrough]];
            case kModernThingThrowableRock:
                pW1->stateID = kCdudeStateAttackBase + 1;
                WeaponRangeSet(pW1, 3000, 11071);
                break;
            default:
                pW1->stateID = kCdudeStateAttackBase + 1;
                WeaponRangeSet(pW1, 5000, 12264);
                break;
        }

        WeaponMeleeSet(pW2);
        pDude->numWeapons++;
    }
    else if (pW1->type == kCdudeWeaponKamikaze)
    {
        const EXPLOSION* pExpl = &explodeInfo[pW1->id - kTrapExploder];
        WeaponRangeSet(pW1, 0, ClipLow(pExpl->radius, 768));
        pW1->angle      = DAngle90;
    }
    else if (pW1->type == kCdudeWeaponSummon)
    {
        pW1->ammo.SetTotal(gGameOptions.nDifficulty + 1);
        pW1->ammo.SetFull();

        WeaponRangeSet(pW1, 2000, kCdudeV1MaxAttackDist);
        pW1->shot.offset.Y  = pDude->pSpr->clipdist;
        pW1->angle          = DAngle90;

        WeaponMeleeSet(pW2);
        pDude->numWeapons++;
    }
    else if (pW1->type == kCdudeWeaponHitscan)
    {
        const VECTORDATA* pVect = &gVectorData[pW1->id];
        WeaponRangeSet(pW1, 0, (pVect->maxDist > 0) ? pVect->maxDist : kCdudeV1MaxAttackDist);

        if (pVect->maxDist > 0 && pVect->maxDist <= 2048)
        {
            pW1->dispersion[0] = pW1->dispersion[1] = 0;
            pW1->angle = pDude->pInfo->Periphery();
        }
        else
        {
            pW1->angle = DAngle::fromBuild(56);
        }
    }
    else if (pW1->type == kCdudeWeaponMissile)
    {
        pW1->angle      = DAngle::fromBuild(56);

        switch (pW1->id)
        {
            case kMissileFireball:
            case kMissileFireballNapalm:
            case kMissileFireballCerberus:
            case kMissileFireballTchernobog:
                WeaponRangeSet(pW1, 3000, kCdudeV1MaxAttackDist);
                WeaponMeleeSet(pW2); pDude->numWeapons++;
                break;
            case kMissileFlareAlt:
                WeaponRangeSet(pW1, 2500, kCdudeV1MaxAttackDist);
                WeaponMeleeSet(pW2); pDude->numWeapons++;
                break;
            case kMissileLifeLeechRegular:
                WeaponRangeSet(pW1, 1500, kCdudeV1MaxAttackDist);
                WeaponMeleeSet(pW2); pDude->numWeapons++;
                break;
            case kMissileFlameSpray:
            case kMissileFlameHound:
                WeaponRangeSet(pW1, 2000, 3500 + (gGameOptions.nDifficulty * 400));
                WeaponMeleeSet(pW2); pDude->numWeapons++;
                pW1->medium = kParMediumLand;
                break;
            default:
                WeaponRangeSet(pW1, 0, kCdudeV1MaxAttackDist);
                break;
        }
    }
}

void CUSTOMDUDEV1_SETUP::SetupBasics(void)
{
    DBloodActor* pSpr = pDude->pSpr;

    if (!pDude->initialized)
    {
        // setup mass
        memset(&pSpr->spriteMass, 0, sizeof(SPRITEMASS));                  // clear mass cache

        auto nPic = pSpr->spr.spritetexture();
        pSpr->spr.setspritetexture(helperGetFirstPic(pDude));                // we need a proper pic to get a proper mass
        pDude->mass = ClipLow(getSpriteMassBySize(pSpr), 40);   // count mass
        pSpr->spr.setspritetexture(nPic);
    }

    // auto clipdist

    
    pSpr->clipdist = clamp(pSpr->spr.scale.Sum() * 8, 2.5, 32.); // ClipRange((pSpr->spr.scale.Sum() * INV_REPEAT_SCALE>yrepeat) >> 1, 10, 128);

}

void CUSTOMDUDEV1_SETUP::SetupDamage(void)
{
    CUSTOMDUDE_WEAPON* pWeap = &pDude->weapons[0];
    bool isMelee = false;

    DamageSetDefault();
    DamageScaleToWeapon(pWeap);
    DamageScaleToSurface(tilesurface(helperGetFirstPic(pDude)));
    DamageScaleToSkill(gGameOptions.nDifficulty);

    switch (pWeap->type)
    {
        case kCdudeWeaponHitscan:
            if (pWeap->GetDistance() > 128.) break;
            [[fallthrough]];
        case kCdudeWeaponKamikaze:
            isMelee = true;
            break;
    }

    if (isMelee)
    {
        // no dodge
        memset(&pDude->dodge, 0, sizeof(CUSTOMDUDE_DODGE));

        // no knockout
        memset(&pDude->knockout, 0, sizeof(CUSTOMDUDE_KNOCKOUT));

        // more dmg and lower chances (isMelee flag analogue)
        pDude->recoil.dmgReq = 25;
        pDude->recoil.chance = 0x0400;
    }
    else
    {
        // average values so we don't have to keep the old code
        pDude->dodge.onDamage.chance = ClipRange((0x6000 / pDude->mass) << 7, 0, 0x10000);
        pDude->dodge.onDamage.dmgReq = 10;

        pDude->recoil.chance  = ClipRange((0x8000 / pDude->mass) << 7, 0, 0x10000);
        pDude->recoil.dmgReq  = 15;
    }
}

void CUSTOMDUDEV1_SETUP::SetupIncarnation(void)
{
    DBloodActor* pSpr = pDude->pSpr;

    if (!pDude->initialized)
    {
        // first make dudes with matching RX to be inactive
        BloodStatIterator it(kStatDude);
        while (auto pSpr2 = it.Next())
        {
            if (pSpr2 != pSpr && pSpr2->hasX() && pSpr2->IsDudeActor())
            {
                if (pSpr2->xspr.rxID == pSpr->xspr.txID)
                {
                    ChangeActorStat(pSpr2, kStatInactive);
                    seqKill(pSpr2);
                }
            }
        }
    }

    pDude->nextDude = false; // then search
    BloodStatIterator it(kStatInactive);
    while (auto pSpr2 = it.Next())
    {
        if (pSpr2 != pSpr && pSpr2->hasX() && pSpr2->IsDudeActor())
        {
            if (pSpr2->xspr.rxID == pSpr->xspr.txID)
            {
                pDude->nextDude = pSpr2;
                if (nnExtRandom(0, 6) == 3) // random stop
                    break;
            }
        }
    }
}

void CUSTOMDUDEV1_SETUP::Setup(void)
{
    assert(pDude != nullptr);

    DBloodActor* pSpr = pDude->pSpr;
    int nBaseSeq = (pSpr->xspr.data2 <= 0) ? kCdudeDefaultSeq : pSpr->xspr.data2;

    /* ---->> PROPER CALL ORDER MATTERS <<---- */

    AnimationConvert(nBaseSeq);             // convert SEQ order from old to a new system
    WeaponConvert(pSpr->xspr.data1);            // convert weapon and give a second one if required
    SoundConvert(pSpr->xspr.sysData1);          // convert RAW order from old to a new system
    VelocitySetDefault(146603);             // default velocity
    SetupBasics();                          // clipdist, mass
    SetupDamage();                          // damage scale, dodge and recoil chances
    
    if (pSpr->xspr.txID)
        SetupIncarnation();
}


void callbackSeqCustom(DBloodActor* pSpr)
{
    if (pSpr->hasX())
    {
        if (rngok(pSpr->xspr.sysData2, 0, countof(gCdudeCustomCallback)))
        {
            auto nFunc = gCdudeCustomCallback[pSpr->xspr.sysData2];
            if (Chance(0x08000))
                evKillActor(pSpr, *nFunc);

            evPostActor(pSpr, 0, *nFunc);
        }
    }
}


static FTextureID helperGetFirstPic(DCustomDude* pDude)
{
    DBloodActor* pSpr = pDude->pSpr;

    FTextureID nPic = pSpr->spr.spritetexture();
    int nSeq = pDude->GetStateSeq(kCdudeStateIdle, kCdudePostureL);
    if (getSequence(nSeq))
    {
        Seq* pSeq = getSequence(nSeq);
        if (pSeq)
        {
            SEQFRAME* pFrame = &pSeq->frames[0];
            nPic = tileGetTextureID(pFrame->tile + (pFrame->tile2 << 12));
        }
    }

    return nPic;
}

static bool helperSeqTriggerExists(int nSeq)
{
    int i;
    Seq* pSeq = getSequence(nSeq);
    if (pSeq)
    {
        i = pSeq->nFrames;
        while (--i >= 0)
        {
            if (pSeq->frames[i].trigger)
                return true;
        }
    }

    return false;
}

static void GetExtents(DBloodActor* pSprite, double* top, double* bottom, FTextureID nPic)
{
    *top = *bottom = pSprite->spr.pos.Z;
    if ((pSprite->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) != CSTAT_SPRITE_ALIGNMENT_FLOOR)
    {
        auto tex = TexMan.GetGameTexture(nPic);
        int height = int(tex->GetDisplayHeight());
        int center = height / 2 + int(tex->GetDisplayTopOffset());
        *top -= pSprite->spr.scale.Y * center;
        *bottom += pSprite->spr.scale.Y * (height - center);
    }
}

static int AreaOfSector(sectortype* pSector)
{
    // the formula makes no sense so keep it precisely as it was
    int area = 0;
    for (auto& wal : pSector->walls)
    {
        auto p1 = wal.pos;
        auto p2 = wal.point2Wall()->pos;
        area += (int(p1.X) + int(p2.X)) * (int(p2.Y) - int(p1.Y));
    }
    area >>= 1;
    return area;
}

static bool isIdKeyword(const char* fullStr, const char* prefix, int* nID)
{
    if (!fullStr || !prefix)
        return false;
    
    int l1 = (int)strlen(fullStr);
    int l2 = (int)strlen(prefix);

    if (l2 < l1 && strnicmp(fullStr, prefix, l2) == 0)
    {
        while (fullStr[l2] == '_')
        {
            if (++l2 >= l1)
                return false;
        }
        
        if (isufix(&fullStr[l2]))
        {
            if (nID)
                *nID = atoi(&fullStr[l2]);

            return true;
        }
    }

    return false;
}


DCustomDude* cdudeGet(DBloodActor* pSpr)
{
    assert(pSpr && pSpr->hasX() && IsCustomDude(pSpr));
    return pSpr->customDude;
}


// for kModernCustomDudeSpawn markers
DBloodActor* cdudeSpawn(DBloodActor* pSource, DBloodActor* pSprite, double nDist)
{
    DVector3 offs(0, nDist, 0);

    DBloodActor* pDude = nnExtSpawnDude(pSprite, kDudeModernCustom, pSprite->spr.pos);

    if (pDude)
    {
        nnExtOffsetSprite(pDude, offs);

        // inherit weapon, seq and sound settings.
        pDude->xspr.data1 = pSource->xspr.data1;
        pDude->xspr.data2 = pSource->xspr.data2;
        pDude->xspr.data3 = pDude->xspr.sysData1 = pSource->xspr.data3; // move sndStartId from data3 to sysData1
        pDude->xspr.data4 = pDude->xspr.sysData2 = pSource->xspr.data4; // health

        // inherit custom hp settings
        pDude->xspr.health = nnExtDudeStartHealth(pDude, pSource->xspr.data4);

        // inherit movement speed.
        pDude->xspr.busyTime = pSource->xspr.busyTime;

        // inherit clipdist?
        if (pSource->clipdist > 0)
            pDude->clipdist = pSource->clipdist;

        if (pSource->spr.flags & kModernTypeFlag1)
        {
            switch (pSource->GetType())
            {
            case kModernCustomDudeSpawn:
                //inherit pal?
                if (pDude->spr.pal <= 0) pDude->spr.pal = pSource->spr.pal;

                // inherit spawn sprite trigger settings, so designer can count monsters.
                pDude->xspr.txID        = pSource->xspr.txID;
                pDude->xspr.command     = pSource->xspr.command;
                pDude->xspr.triggerOn   = pSource->xspr.triggerOn;
                pDude->xspr.triggerOff  = pSource->xspr.triggerOff;

                // inherit drop items
                pDude->xspr.dropMsg = pSource->xspr.dropMsg;

                // inherit required key so it can be dropped
                pDude->xspr.key = pSource->xspr.key;

                // inherit dude flags
                pDude->xspr.dudeDeaf    = pSource->xspr.dudeDeaf;
                pDude->xspr.dudeGuard   = pSource->xspr.dudeGuard;
                pDude->xspr.dudeAmbush  = pSource->xspr.dudeAmbush;
                pDude->xspr.dudeFlag4   = pSource->xspr.dudeFlag4;
                pDude->xspr.modernFlags     = pSource->xspr.modernFlags;
                break;
            }
        }

        // inherit sprite size (useful for seqs with zero repeats)
        if (pSource->spr.flags & kModernTypeFlag2)
        {
            pDude->spr.scale = pSource->spr.scale;
        }

        if (AllowedKillType(pDude))
            Level.addKillCount();
        aiInitSprite(pDude);
    }

    return pDude;
}


// for kThingModernEnemyLifeLeech
void cdudeLeechOperate(DBloodActor* pSpr)
{
    if (DBloodActor* pOwn = pSpr->ownerActor)
    {
        if (xsprIsFine(pOwn) && pOwn->IsDudeActor())
        {
            if (pOwn->xspr.target)
            {
                pSpr->xspr.target = pOwn->xspr.target;
            }
            else if (DBloodActor* pTarget = pSpr->xspr.target)
            {
                aiSetTarget(pOwn, pTarget->spr.pos);
                aiActivateDude(pOwn);
            }
        }
    }

    DBloodActor* pTarg = pSpr->xspr.target;
    if (pTarg && pTarg != pSpr->ownerActor)
    {
        if (pTarg->spr.statnum == kStatDude && xsprIsFine(pTarg) && !pSpr->xspr.stateTimer)
        {
            if (pTarg->IsPlayerActor())
            {
                DBloodPlayer* pPlayer = getPlayer(pTarg);
                if (powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
                    return;
            }

            auto targpos = pTarg->spr.pos;

            double zTop, zBot;
            GetActorExtents(pSpr, &zTop, &zBot);
            double nDist = (targpos - pSpr->spr.pos.XY()).Length();
            if (nDist && cansee(DVector3(pSpr->spr.pos.XY(), zTop), pSpr->sector(), targpos, pTarg->sector()))
            {
                targpos += pTarg->vel.XY() * nDist * 0.0375;
                DAngle nAng = (targpos - pSpr->spr.pos.XY()).Angle();
                DVector3 dv;
                dv.XY() = (nAng.ToVector() * 64);
                dv.Z = (pTarg->spr.pos.Z - zTop / nDist) * 4;
                
                int nMissileType = kMissileLifeLeechAltNormal + (pSpr->xspr.data3 ? 1 : 0);
                int t2;

                if (!pSpr->xspr.data3) t2 = 120 / 10;
                else t2 = (3 * 120) / 10;

                DBloodActor* pMissile = actFireMissile(pSpr, 0, zTop - pSpr->spr.pos.Z - 1.5, dv, nMissileType);
                if (pMissile)
                {
                    pMissile->ownerActor = pSpr->ownerActor;
                    pSpr->xspr.stateTimer = 1;
                    evPostActor(pSpr, t2, AF(LeechStateTimer));
                    pSpr->xspr.data3 = ClipLow(pSpr->xspr.data3 - 1, 0);
                }
            }
        }
    }
}

// Make the custom dude a DObject so that it can be fully serialized and GC'd.
IMPLEMENT_CLASS(DCustomDude, false, true)
IMPLEMENT_POINTERS_START(DCustomDude)
IMPLEMENT_POINTER(pSpr)
IMPLEMENT_POINTER(pLeech)
IMPLEMENT_POINTERS_END

size_t DCustomDude::PropagateMark()
{
    if (auto ppDude = std::get_if<DBloodActor*>(&nextDude))
    {
        GC::Mark(*ppDude);
    }
    for (auto& slave : pSlaves) GC::Mark(slave);
    return pSlaves.Size() + Super::PropagateMark();
}

void DCustomDude::OnDestroy()
{
    // free all allocations now and sever the link to the dude actor.
    pSpr = nullptr;
    pSlaves.Reset();
    for (auto& ef : effects)
    {
        ef.pAnims.Reset();
        ef.pFrames.Reset();
        ef.pStates.Reset();
    }
    Super::Destroy();
}


END_BLD_NS
#endif