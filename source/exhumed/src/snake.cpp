
#include "engine.h"
#include "exhumed.h"
#include "snake.h"
#include "status.h"
#include "player.h"
#include "runlist.h"
#include "sequence.h"
#include "bullet.h"
#include "input.h"
#include "anims.h"
#include "lighting.h"
#include "sound.h"
#include "move.h"
#include "trigdat.h"
#include "gun.h"
#include <string.h>
#include <assert.h>

#define kMaxSnakes	50

int nSnakeCount = 0;
int nSnakesFree;

short SnakeFree[kMaxSnakes];
short nPlayerSnake[kMaxPlayers];

Snake SnakeList[kMaxSnakes];
short nSnakePlayer[kMaxSnakes];


void InitSnakes()
{
    nSnakeCount = 0;

    for (int i = 0; i < kMaxSnakes; i++) {
        SnakeFree[i] = i;
    }

    nSnakesFree = kMaxSnakes;
    memset(nPlayerSnake, 0, sizeof(nPlayerSnake));
}

short GrabSnake()
{
    nSnakesFree--;
    return SnakeFree[nSnakesFree];
}

void DestroySnake(int nSnake)
{
    short nRun = SnakeList[nSnake].nRun;
    runlist_SubRunRec(nRun);

    for (int i = 0; i < kSnakeSprites; i++)
    {
        short nSprite = SnakeList[nSnake].nSprites[i];

        runlist_DoSubRunRec(sprite[nSprite].lotag - 1);
        runlist_DoSubRunRec(sprite[nSprite].owner);

        mydeletesprite(nSprite);
    }

    SnakeFree[nSnakesFree] = nSnake;
    nSnakesFree++;

    if (nSnake == nSnakeCam)
    {
        nSnakeCam = -1;
        if (!bFullScreen) {
            RefreshStatus();
        }
    }
}

void ExplodeSnakeSprite(int nSprite, short nPlayer)
{
    short nDamage = BulletInfo[kWeaponStaff].nDamage;

    if (nPlayerDouble[nPlayer] > 0) {
        nDamage *= 2;
    }

    // take a copy of this, to revert after call to runlist_RadialDamageEnemy()
    short nOwner = sprite[nSprite].owner;
    sprite[nSprite].owner = PlayerList[nPlayer].nSprite;

    runlist_RadialDamageEnemy(nSprite, nDamage, BulletInfo[kWeaponStaff].field_10);

    sprite[nSprite].owner = nOwner;

    BuildAnim(-1, 23, 0, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, sprite[nSprite].sectnum, 40, 4);

    AddFlash(sprite[nSprite].sectnum, sprite[nSprite].x, sprite[nSprite].y, sprite[nSprite].z, 128);

    StopSpriteSound(nSprite);
}

int BuildSnake(short nPlayer, short zVal)
{
    if (!nSnakesFree)
        return -1;

    zVal -= 1280;

    short nPlayerSprite = PlayerList[nPlayer].nSprite;
    short nViewSect = nPlayerViewSect[nPlayer];
    short nPic = seq_GetSeqPicnum(kSeqSnakBody, 0, 0);

    int x = sprite[nPlayerSprite].x;
    int y = sprite[nPlayerSprite].y;
    int z = (sprite[nPlayerSprite].z + zVal) - 2560;
    short nAngle = sprite[nPlayerSprite].ang;

    short hitsect, hitwall, hitsprite;
    int hitx, hity, hitz;

    short nSprite;

    vec3_t pos = { x, y, z };
    hitdata_t hitData;
    hitscan(&pos, sprite[nPlayerSprite].sectnum, Sin(nAngle + 512), Sin(nAngle), 0, &hitData, CLIPMASK1);

    hitx = hitData.pos.x;
    hity = hitData.pos.y;
    hitz = hitData.pos.z;
    hitsect = hitData.sect;
    hitwall = hitData.wall;
    hitsprite = hitData.sprite;
    hitsprite = hitData.sprite;

    int nSqrt = ksqrt(((hity - y) * (hity - y)) + ((hitx - x) * (hitx - x)));

    if (nSqrt < sintable[512] >> 4)
    {
        BackUpBullet(&hitx, &hity, nAngle);
        nSprite = insertsprite(hitsect, 202);
        sprite[nSprite].x = hitx;
        sprite[nSprite].y = hity;
        sprite[nSprite].z = hitz;

        ExplodeSnakeSprite(nSprite, nPlayer);
        mydeletesprite(nSprite);
        return -1;
    }
    else
    {
        short nTarget;

        if (hitsprite < 0 || sprite[hitsprite].statnum < 90 || sprite[hitsprite].statnum > 199) {
            nTarget = sPlayerInput[nPlayer].nTarget;
        }
        else {
            nTarget = hitsprite;
        }

        short nSnake = GrabSnake();
//		int var_40 = 0;

        uint8_t var_18 = 0; // var_40 + var_40; // CHECKME ??

//		GrabTimeSlot(3);

        short var_24;

        for (int i = 0; i < kSnakeSprites; i++)
        {
            nSprite = insertsprite(nViewSect, 202);
            assert(nSprite >= 0 && nSprite < kMaxSprites);

            sprite[nSprite].owner = nPlayerSprite;
            sprite[nSprite].picnum = nPic;

            if (i != 0)
            {
                sprite[nSprite].x = sprite[var_24].x;
                sprite[nSprite].y = sprite[var_24].y;
                sprite[nSprite].z = sprite[var_24].z;
                sprite[nSprite].xrepeat = 40 - (var_18 + i);
                sprite[nSprite].yrepeat = 40 - (var_18 + i);
            }
            else
            {
                sprite[nSprite].x = sprite[nPlayerSprite].x;
                sprite[nSprite].y = sprite[nPlayerSprite].y;
                sprite[nSprite].z = sprite[nPlayerSprite].z + zVal;
                sprite[nSprite].xrepeat = 32;
                sprite[nSprite].yrepeat = 32;
                nViewSect = sprite[nSprite].sectnum;
                var_24 = nSprite;
            }

            sprite[nSprite].clipdist = 10;
            sprite[nSprite].cstat = 0;
            sprite[nSprite].shade = -64;
            sprite[nSprite].pal = 0;
            sprite[nSprite].xoffset = 0;
            sprite[nSprite].yoffset = 0;
            sprite[nSprite].ang = sprite[nPlayerSprite].ang;
            sprite[nSprite].xvel = 0;
            sprite[nSprite].yvel = 0;
            sprite[nSprite].zvel = 0;
            sprite[nSprite].hitag = 0;
            sprite[nSprite].extra = -1;
            sprite[nSprite].lotag = runlist_HeadRun() + 1;

            SnakeList[nSnake].nSprites[i] = nSprite;

            sprite[nSprite].owner = runlist_AddRunRec(sprite[nSprite].lotag - 1, ((nSnake << 8) | i) | 0x110000);

            var_18 += 2;
//			var_40++;
        }

        SnakeList[nSnake].nRun = runlist_AddRunRec(NewRun, nSnake | 0x110000);
        SnakeList[nSnake].c[1] = 2;
        SnakeList[nSnake].c[5] = 5;
        SnakeList[nSnake].c[2] = 4;
        SnakeList[nSnake].c[3] = 6;
        SnakeList[nSnake].c[4] = 7;
        SnakeList[nSnake].c[6] = 6;
        SnakeList[nSnake].c[7] = 7;
        SnakeList[nSnake].nEnemy = nTarget;
        SnakeList[nSnake].sC = 1200;
        SnakeList[nSnake].sE = 0;
        nSnakePlayer[nSnake] = nPlayer;
        nPlayerSnake[nPlayer] = nSnake;

        if (bSnakeCam)
        {
            if (nSnakeCam < 0) {
                nSnakeCam = nSnake;
            }
        }

        D3PlayFX(StaticSound[kSound6], var_24);
    }

    return nSprite;
}

int FindSnakeEnemy(short nSnake)
{
    short nPlayer = nSnakePlayer[nSnake];
    short nPlayerSprite = PlayerList[nPlayer].nSprite;

    short nSprite = SnakeList[nSnake].nSprites[0]; // CHECKME

    short nAngle = sprite[nSprite].ang;
    short nSector = sprite[nSprite].sectnum;

    int esi = 2048;

    int nEnemy = -1;

    for (int i = headspritesect[nSector]; i >= 0; i = nextspritesect[i])
    {
        if (sprite[i].statnum >= 90 && sprite[i].statnum < 150 && sprite[i].cstat & 0x101)
        {
            if (i != nPlayerSprite && (!(sprite[i].cstat & 0x8000)))
            {
                int nAngle2 = (nAngle - GetAngleToSprite(nSprite, i)) & kAngleMask;
                if (nAngle2 < esi)
                {
                    nEnemy = i;
                    esi = nAngle2;
                }
            }
        }
    }

    if (nEnemy == -1)
    {
        SnakeList[nSnake].nEnemy--;
        if (SnakeList[nSnake].nEnemy < -25)
        {
            nEnemy = nPlayerSprite;
            SnakeList[nSnake].nEnemy = nPlayerSprite;
        }
    }
    else
    {
        SnakeList[nSnake].nEnemy = nEnemy;
    }

    return nEnemy;
}

void FuncSnake(int a, int nDamage, int nRun)
{
    int nMessage = a & 0x7F0000;

    switch (nMessage)
    {
        default:
        {
            DebugOut("unknown msg %x for bullet\n", a & 0x7F0000);
            return;
        }

        case 0x90000:
        {
            short nSnake = RunData[nRun].nVal;
            short nSprite = a & 0xFFFF;

            if (nSnake & 0xFF) {
                seq_PlotSequence(nSprite, SeqOffsets[kSeqSnakBody], 0, 0);
            }
            else {
                seq_PlotSequence(nSprite, SeqOffsets[kSeqSnakehed], 0, 0);
            }

            tsprite[nSprite].owner = -1;
            return;
        }

        case 0xA0000:
        {
            return;
        }

        case 0x20000:
        {
            short nSnake = RunData[nRun].nVal;
            assert(nSnake >= 0 && nSnake < kMaxSnakes);

            short nSprite = SnakeList[nSnake].nSprites[0];

            seq_MoveSequence(nSprite, SeqOffsets[kSeqSnakehed], 0);

            short nEnemySprite = SnakeList[nSnake].nEnemy;

            int nMov;
            int zVal;

            if (nEnemySprite >= 0 && (sprite[nEnemySprite].cstat & 0x101))
            {
                zVal = sprite[nSprite].z;

                nMov = AngleChase(nSprite, nEnemySprite, 1200, SnakeList[nSnake].sE, 32);

                zVal = sprite[nSprite].z - zVal;
            }
            else
            {
                if (!(sprite[nEnemySprite].cstat & 0x101)) {
                    SnakeList[nSnake].nEnemy = -1;
                }

                nMov = movesprite(nSprite,
                    600 * Sin(sprite[nSprite].ang + 512),
                    600 * Sin(sprite[nSprite].ang),
                    Sin(SnakeList[nSnake].sE) >> 5,
                    0, 0, CLIPMASK1);

                FindSnakeEnemy(nSnake);

                zVal = 0;
            }

            if (nMov)
            {
                short nPlayer = nSnakePlayer[nSnake];
                ExplodeSnakeSprite(SnakeList[nSnake].nSprites[0], nPlayer);
                
                nPlayerSnake[nPlayer] = -1;
                nSnakePlayer[nSnake] = -1;

                DestroySnake(nSnake);
                return;
            }
            else
            {
                short nAngle = sprite[nSprite].ang;
                int var_30 = -(64 * Sin(nAngle + 512));
                int var_34 = -(64 * Sin(nAngle));

                int var_40 = var_30 + (var_30 * 8);
                int var_4C = var_30 + (var_34 * 8);

                int var_20 = SnakeList[nSnake].sE;

                SnakeList[nSnake].sE = (SnakeList[nSnake].sE + 64) & 0x7FF;

                int var_28 = (nAngle + 512) & kAngleMask;
                short nSector = sprite[nSprite].sectnum;

                int x = sprite[nSprite].x;
                int y = sprite[nSprite].y;
                int z = sprite[nSprite].z;

                int ebp = -(zVal * 7);

                int var_18 = var_28;

                for (int i = 7; i > 0; i--)
                {
                    int nSprite2 = SnakeList[nSnake].nSprites[i];

                    sprite[nSprite2].ang = nAngle;
                    sprite[nSprite2].x = x;
                    sprite[nSprite2].y = y;
                    sprite[nSprite2].z = z;

                    mychangespritesect(nSprite2, nSector);

                    var_40 = var_40 - var_30;
                    var_4C = var_4C - var_34;

                    ebp += zVal;

                    int eax = Sin(var_20) * SnakeList[nSnake].c[i];
                    eax >>= 9;

                    int ecx = eax * Sin(var_28 + 512);

                    eax = sintable[var_18] * eax;

                    movesprite(nSprite2, var_40 + ecx, var_4C + eax, ebp, 0, 0, CLIPMASK1);
            
                    var_20 = (var_20 + 128) & kAngleMask;
                }
            }
        }
    }
}
