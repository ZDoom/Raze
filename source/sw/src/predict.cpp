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

//#define MAIN
#define QUIET
#include "build.h"

#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
//#include "quake.h"
//#include "jsector.h"

//#include "mytypes.h"
//#include "control.h"
//#include "function.h"
#include "network.h"
//#include "symutil.h"

SWBOOL PredictionOn = TRUE;
SWBOOL Prediction = FALSE;
PLAYER PredictPlayer;
USER PredictUser;
PLAYERp ppp = &PredictPlayer;

typedef struct
{
    int x,y,z,horiz;
    short ang,filler;
} PREDICT, *PREDICTp;

PREDICT Predict[MOVEFIFOSIZ];
int predictmovefifoplc;

void DoPlayerSectorUpdatePreMove(PLAYERp);
void DoPlayerSectorUpdatePostMove(PLAYERp);

extern SWBOOL GamePaused;

#define PREDICT_DEBUG 0

#if PREDICT_DEBUG
void (*pred_last_func)(PLAYERp) = NULL;
#endif

void
InitPrediction(PLAYERp pp)
{
    if (!PredictionOn)
        return;

#if PREDICT_DEBUG
    pred_last_func = pp->DoPlayerAction;
#endif

    // make a copy of player struct and sprite
    *ppp = *pp;
    PredictUser = *User[pp->PlayerSprite];
}

#if PREDICT_DEBUG
PredictDebug(PLAYERp ppp)
{
    static FILE *fout = NULL;
    static char pred_sym_name[80];

    if (SymCountCode == 0)
        LoadSymTable("swcode.sym", &SymTableCode, &SymCountCode);

    if (SymCountCode <= 0)
        return;

    if (!fout)
    {
        if ((fout = fopen("dbgpred.txt", "wb")) == NULL)
            return;
    }

    if (ppp->DoPlayerAction != pred_last_func)
    {
        extern uint32_t MoveThingsCount;
        SYM_TABLEp st_ptr;
        uint32_t unrelocated_offset;
        uint32_t offset_from_symbol;

        unrelocated_offset = SymCodePtrToOffset((void *)ppp->DoPlayerAction);
        st_ptr = SearchSymTableByOffset(SymTableCode, SymCountCode, unrelocated_offset, &offset_from_symbol);
        ASSERT(st_ptr);
        strcpy(pred_sym_name, st_ptr->Name);

        fprintf(fout, "%s, %d\n", pred_sym_name, MoveThingsCount);
    }
}
#endif


void
DoPrediction(PLAYERp ppp)
{
    USERp u;
    SPRITE spr;
    int bakrandomseed;
    short angvel;

    // routine called from MoveLoop

    if (!PredictionOn)
        return;

    ppp->input = Player[myconnectindex].inputfifo[predictmovefifoplc & (MOVEFIFOSIZ-1)];

    // get rid of input bits so it doesn't go into other code branches that would
    // get it out of sync
    RESET(ppp->input.bits,
          BIT(SK_SHOOT)|BIT(SK_OPERATE)|BIT(SK_INV_LEFT)|BIT(SK_INV_RIGHT)|
          BIT(SK_INV_USE)|BIT(SK_HIDE_WEAPON)|
          BIT(SK_AUTO_AIM)|
          BIT(SK_CENTER_VIEW)|
          SK_WEAPON_MASK|
          SK_INV_HOTKEY_MASK
          );

    SET(ppp->KeyPressFlags,
        BIT(SK_SHOOT)|BIT(SK_OPERATE)|BIT(SK_INV_LEFT)|BIT(SK_INV_RIGHT)|
        BIT(SK_INV_USE)|BIT(SK_HIDE_WEAPON)|
        BIT(SK_AUTO_AIM)|
        BIT(SK_CENTER_VIEW)|
        SK_WEAPON_MASK|
        SK_INV_HOTKEY_MASK
        );

    // back up things so they won't get stepped on
    bakrandomseed = randomseed;
    spr = sprite[Player[myconnectindex].PlayerSprite];
    sprite[Player[myconnectindex].PlayerSprite].cstat = 0;

    u = User[ppp->PlayerSprite];
    User[ppp->PlayerSprite] = &PredictUser;

    ppp->oang = ppp->pang;
    ppp->oposx = ppp->posx;
    ppp->oposy = ppp->posy;
    ppp->oposz = ppp->posz;
    ppp->ohoriz = ppp->horiz;

#if PREDICT_DEBUG
    PredictDebug(ppp);
#endif

    // go through the player MOVEMENT code only
    Prediction = TRUE;
    DoPlayerSectorUpdatePreMove(ppp);
    (*ppp->DoPlayerAction)(ppp);
    DoPlayerSectorUpdatePostMove(ppp);
    Prediction = FALSE;

    // restore things
    User[ppp->PlayerSprite] = u;
    sprite[Player[myconnectindex].PlayerSprite] = spr;
    randomseed = bakrandomseed;

    Predict[predictmovefifoplc & (MOVEFIFOSIZ-1)].ang = ppp->pang;
    Predict[predictmovefifoplc & (MOVEFIFOSIZ-1)].x = ppp->posx;
    Predict[predictmovefifoplc & (MOVEFIFOSIZ-1)].y = ppp->posy;
    Predict[predictmovefifoplc & (MOVEFIFOSIZ-1)].z = ppp->posz;
    Predict[predictmovefifoplc & (MOVEFIFOSIZ-1)].horiz = ppp->horiz;
    predictmovefifoplc++;
}

void
CorrectPrediction(int actualfifoplc)
{
    PREDICTp predict = &Predict[actualfifoplc & (MOVEFIFOSIZ-1)];

    if (!PredictionOn)
        return;

    if (!CommEnabled)
        return;

    // see if player position is predicted position
    if (predict->ang == Player[myconnectindex].pang &&
        predict->x == Player[myconnectindex].posx &&
        predict->y == Player[myconnectindex].posy &&
        predict->z == Player[myconnectindex].posz &&
        predict->horiz == Player[myconnectindex].horiz
        )
    {
        return;
    }

//    //DSPRINTF(ds,"PREDICT ERROR: %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld", predict->ang,  Player[myconnectindex].pang, predict->x,    Player[myconnectindex].posx, predict->y,    Player[myconnectindex].posy, predict->z,    Player[myconnectindex].posz,  predict->horiz,Player[myconnectindex].horiz);
//    MONO_PRINT(ds);

    InitPrediction(&Player[myconnectindex]);
    // puts the predicted pos back to actual pos
    predictmovefifoplc = movefifoplc;

    while (predictmovefifoplc < Player[myconnectindex].movefifoend)
    {
        DoPrediction(ppp);
    }
}

