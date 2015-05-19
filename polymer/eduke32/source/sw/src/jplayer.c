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

// JPLAYER.C
// Copyright (c) 1996 by Jim Norwood

#include "build.h"

#include "mytypes.h"
#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "player.h"
#include "lists.h"
#include "warp.h"
#include "quake.h"

#include "common_game.h"
#include "function.h"
#include "control.h"
#include "trigger.h"

#include "savedef.h"
#include "menus.h"
#include "net.h"
#include "pal.h"

#include "bots.h"

SWBOOL WeaponOK(PLAYERp pp);

#define MAXANGVEL 80

// From build.h
#define CLIPMASK0 (((1L)<<16)+1L)
#define CLIPMASK1 (((256L)<<16)+64L)


// PLAYER QUOTES TO OTHER PLAYERS ////////////////////////////////////////////////////////////

#define STARTALPHANUM 4608  // New SW font for typing in stuff, It's in ASCII order.
#define ENDALPHANUM   4701
#define MINIFONT      2930  // Start of small font, it's blue for good palette swapping

#define NUMPAGES 1
#define NUMOFFIRSTTIMEACTIVE 100  // You can save up to 100 strings in the message history queue

char pus, pub;  // Global text vars
char fta_quotes[NUMOFFIRSTTIMEACTIVE][64];


int gametext(int x,int y,char *t,char s,short dabits)
{
    short ac,newx;
    char centre, *oldt;

    centre = (x == (320>>1));
    newx = 0;
    oldt = t;

    if (centre)
    {
        while (*t)
        {
            if (*t == 32) {newx+=5; t++; continue; }
            else ac = *t - '!' + STARTALPHANUM;

            if (ac < STARTALPHANUM || ac > ENDALPHANUM) break;

            if (*t >= '0' && *t <= '9')
                newx += 8;
            else newx += tilesiz[ac].x;
            t++;
        }

        t = oldt;
        x = (320>>1)-(newx>>1);
    }

    while (*t)
    {
        if (*t == 32) {x+=5; t++; continue; }
        else ac = *t - '!' + STARTALPHANUM;

        if (ac < STARTALPHANUM || ac > ENDALPHANUM)
            break;

        rotatesprite(x<<16,y<<16,65536L,0,ac,s,16,dabits,0,0,xdim-1,ydim-1);

        if (*t >= '0' && *t <= '9')
            x += 8;
        else x += tilesiz[ac].x;

        t++;
    }

    return x;
}

int minigametext(int x,int y,char *t,char s,short dabits)
{
    short ac,newx;
    char centre, *oldt;

    centre = (x == (320>>1));
    newx = 0;
    oldt = t;

    if (centre)
    {
        while (*t)
        {
            if (*t == 32) {newx+=4; t++; continue; }
            else ac = *t - '!' + 2930;

            if ((ac < 2930 || ac > 3023) && *t != asc_Space) break;

            if (*t > asc_Space && *t < 127)
            {
                newx += tilesiz[ac].x;
            }
            else
                x += 4;

            t++;
        }

        t = oldt;
        x = (320>>1)-(newx>>1);
    }

    while (*t)
    {
        if (*t == 32) {x+=4; t++; continue; }
        else ac = *t - '!' + 2930;

        if ((ac < 2930 || ac > 3023) && *t != asc_Space) break;

        if (*t > asc_Space && *t < 127)
        {
            rotatesprite(x<<16,y<<16,65536L,0,ac,-128,17,dabits,0,0,xdim-1,ydim-1);
            x += tilesiz[ac].x;
        }
        else
            x += 4;

        t++;
    }

    return x;
}

int minitext(int x,int y,char *t,char p,char sb)
{
    short ac;

    while (*t)
    {
        *t = toupper(*t);
        if (*t == 32) {x+=5; t++; continue; }
        else ac = *t - '!' + MINIFONT;

        rotatesprite(x<<16,y<<16,65536L,0,ac,0,p,sb,0,0,xdim-1,ydim-1);
        x += 4; // tilesiz[ac].x+1;

        t++;
    }
    return x;
}

int minitextshade(int x,int y,char *t,char s,char p,char sb)
{
    short ac;

    while (*t)
    {
        *t = toupper(*t);
        if (*t == 32) {x+=5; t++; continue; }
        else ac = *t - '!' + MINIFONT;

        rotatesprite(x<<16,y<<16,65536L,0,ac,s,p,sb,0,0,xdim-1,ydim-1);
        x += 4; // tilesiz[ac].x+1;

        t++;
    }
    return x;
}

int quotebot, quotebotgoal;
short user_quote_time[MAXUSERQUOTES];
char user_quote[MAXUSERQUOTES][256];

void adduserquote(char *daquote)
{
    int i;

    SetRedrawScreen(Player+myconnectindex);

    for (i=MAXUSERQUOTES-1; i>0; i--)
    {
        strcpy(user_quote[i],user_quote[i-1]);
        user_quote_time[i] = user_quote_time[i-1];
    }
    strcpy(user_quote[0],daquote);
    user_quote_time[0] = 180;
}

void operatefta(void)
{
    int i, j, k;

    j=MESSAGE_LINE; // Base line position on screen
    quotebot = min(quotebot,j);
    quotebotgoal = min(quotebotgoal,j);
    if (MessageInputMode)
        j -= 6; // Bump all lines up one to make room for new line
    quotebotgoal = j;
    j = quotebot;

    for (i=0; i<MAXUSERQUOTES; i++)
    {
        k = user_quote_time[i];
        if (k <= 0)
            break;

        if (gs.BorderNum <= BORDER_BAR+1)
        {
            // dont fade out
            if (k > 4)
                minigametext(320>>1,j,user_quote[i],0,2+8);
            else if (k > 2)
                minigametext(320>>1,j,user_quote[i],0,2+8+1);
            else
                minigametext(320>>1,j,user_quote[i],0,2+8+1+32);
        }
        else
        {
            // dont fade out
            minigametext(320>>1,j,user_quote[i],0,2+8);
        }

        j -= 6;
    }
}

//////////// Console Message Queue ////////////////////////////////////
int conbot, conbotgoal;
char con_quote[MAXCONQUOTES][256];

void addconquote(char *daquote)
{
    int i;

    for (i=MAXCONQUOTES-1; i>0; i--)
    {
        strcpy(con_quote[i],con_quote[i-1]);
    }
    strcpy(con_quote[0],daquote);
}

#define CON_ROT_FLAGS (ROTATE_SPRITE_CORNER|ROTATE_SPRITE_SCREEN_CLIP|ROTATE_SPRITE_NON_MASK)
void operateconfta(void)
{
    int i, j, k;

    if (!ConPanel) return; // If panel isn't up, don't draw anything

    // Draw the background console pic
    rotatesprite((0)<<16,(0)<<16,65536L,0,5119,0,0,CON_ROT_FLAGS,0,0,xdim-1,ydim-1);

    j=99; // Base line position on screen
    conbot = min(conbot,j);
    conbotgoal = min(conbotgoal,j);
    if (ConInputMode) j -= 6; // Bump all lines up one to make room for new line
    conbotgoal = j; j = conbot;

    for (i=0; i<MAXCONQUOTES; i++)
    {
        MNU_DrawSmallString(27, j, con_quote[i], 0, 17); // 17 = white
        j -= 6;
    }
}

// BOT STUFF ////////////////////////////////////////////////////////////////////////////////

void BOT_UseInventory(PLAYERp p, short targetitem, SW_PACKET *syn)
{
    // Try to get to item
    if (p->InventoryNum == targetitem)
        syn->bits |= (1<<SK_INV_USE);
    else
    {
        syn->bits |= (1<<SK_INV_LEFT);  // Scroll to it
        syn->bits |= (1<<SK_INV_USE); // Use whatever you're on too
    }
}

void BOT_ChooseWeapon(PLAYERp p, USERp u, SW_PACKET *syn)
{
    short weap;

    // If you have a nuke, fire it
    if (u->WeaponNum == WPN_MICRO && p->WpnRocketNuke && p->WpnRocketType != 2)
    {
        syn->bits ^= 15;
        syn->bits |= 4;
    }
    else
        for (weap=9; weap>=0; weap--)
        {
            if (weap <= u->WeaponNum) break;
            if (TEST(p->WpnFlags, BIT(weap)) && p->WpnAmmo[weap] > DamageData[weap].min_ammo)
            {
                syn->bits ^= 15;
                syn->bits |= weap;
                break;
            }
        }
}

int getspritescore(int snum, int dapicnum)
{

    switch (dapicnum)
    {
    case ICON_STAR: return 5;
    case ICON_UZI: return 20;
    case ICON_UZIFLOOR: return 20;
    case ICON_LG_UZI_AMMO: return 15;
    case ICON_HEART: return 160;
    case ICON_HEART_LG_AMMO: return 60;
    case ICON_GUARD_HEAD: return 170;
    case ICON_FIREBALL_LG_AMMO: return 70;
    case ICON_ROCKET: return 100;
    case ICON_SHOTGUN: return 130;
    case ICON_LG_ROCKET: return 100;
    case ICON_LG_SHOTSHELL: return 30;
    case ICON_MICRO_GUN: return 200;
    case ICON_MICRO_BATTERY: return 100;
    case ICON_GRENADE_LAUNCHER: return 150;
    case ICON_LG_GRENADE: return 50;
    case ICON_LG_MINE: return 150;
    case ICON_RAIL_GUN: return 180;
    case ICON_RAIL_AMMO: return 80;

    case ST_QUICK_EXIT:
    case ST_QUICK_SCAN:
    case ICON_MEDKIT:
    case ICON_CHEMBOMB:
    case ICON_FLASHBOMB:
    case ICON_NUKE:
    case ICON_CALTROPS:
    case TRACK_SPRITE:
    case ST1:
    case ST2:
    case ST_QUICK_JUMP:
    case ST_QUICK_JUMP_DOWN:
    case ST_QUICK_SUPER_JUMP: return 120;  break;

        // Commented out for now, example.
//        case FREEZEAMMO: if (ps[snum].ammo_amount[FREEZE_WEAPON] < max_ammo_amount[FREEZE_WEAPON]) return(10); else return(0);
    }


    return 0;
}

static int fdmatrix[13][13] =
{
//SWRD  SHUR  UZI SHOT RPG  40MM MINE RAIL HEAD HEAD2HEAD3HEART
    { 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},   //SWRD
    {1024, 512, 128, 128,2560, 128,2560, 128,2560,2560,2560, 128, 128},   //SHUR
    {2560,1024, 512, 512,2560, 128,2560,2560,1024,2560,2560,2560,2560},   //UZI
    { 512, 512, 512, 512,2560, 128,2560, 512, 512, 512, 512, 512, 512},   //SHOT
    {2560,2560,2560,2560,2560,2560,2560,2560,2560,2560,2560,2560,2560},   //RPG
    { 512, 512, 512, 512,2048, 512,2560,2560, 512,2560,2560,2560,2560},   //40MM
    { 128, 128, 128, 128, 512, 128, 128, 128, 128, 128, 128, 128, 128},   //MINE
    {1536,1536,1536,1536,2560,1536,1536,1536,1536,1536,1536,1536,1536},   //RAIL
    {2560,1024, 512,1024,1024,1024,2560, 512,1024,2560,2560, 512, 512},   //HEAD1
    { 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},   //HEAD2
    { 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512},   //HEAD3
    {1024, 512, 128, 128,2560, 512,2560,1024, 128,2560,1024,1024,1024},   //HEART
};

static int goalx[MAX_SW_PLAYERS_REG], goaly[MAX_SW_PLAYERS_REG], goalz[MAX_SW_PLAYERS_REG];
static int goalsect[MAX_SW_PLAYERS_REG], goalwall[MAX_SW_PLAYERS_REG], goalsprite[MAX_SW_PLAYERS_REG];
static int goalplayer[MAX_SW_PLAYERS_REG], clipmovecount[MAX_SW_PLAYERS_REG];
short searchsect[MAXSECTORS], searchparent[MAXSECTORS];
char dashow2dsector[(MAXSECTORS+7)>>3];

void computergetinput(int snum, SW_PACKET *syn)
{
    int i, j, k, l, x1, y1, z1, x2, y2, z2, dx, dy, nextj;
    int dist, daang, zang, fightdist, damyang, damysect;
    int startsect, endsect, splc, send, startwall, endwall;
    hitdata_t hitinfo;
    PLAYERp p;
    walltype *wal;
    int myx, myy, myz, myang, mycursectnum;
    USERp u;
    short weap;
    //extern SWBOOL Pachinko_Win_Cheat;

    if (!MoveSkip4) return; // Make it so the bots don't slow the game down so bad!

    p = &Player[snum];
    u = User[p->PlayerSprite];  // Set user struct

    // Copy current weapon number to player struct
    p->WpnNum = u->WeaponNum;
    if (p->WpnNum >= MAX_WEAPONS) p->WpnNum = MAX_WEAPONS-1;

    // Init local position variables
    myx = p->posx;
    myy = p->posy;
    myz = p->posz;
    myang = p->pang;
    mycursectnum = p->cursectnum;

    // Reset input bits
    syn->vel = 0;
    syn->svel = 0;
    syn->angvel = 0;
    syn->aimvel = 0;
    syn->bits = 0;

    x1 = p->posx;
    y1 = p->posy;
    z1 = p->posz;

    damyang = p->pang;
    damysect = sprite[p->PlayerSprite].sectnum;
    if ((numplayers >= 2) && (snum == myconnectindex))
    { x1 = myx; y1 = myy; z1 = myz+PLAYER_HEIGHT; damyang = myang; damysect = mycursectnum; }

    // Always operate everything
    syn->bits |= (1<<SK_OPERATE);

    // If bot can't see the goal enemy, set target to himself so that he
    // will pick a new target
    if (TEST(Player[goalplayer[snum]].Flags, PF_DEAD) || STD_RANDOM_RANGE(1000) > 800)
        goalplayer[snum] = snum;
    else
    {
        x2 = Player[goalplayer[snum]].posx;
        y2 = Player[goalplayer[snum]].posy;
        z2 = Player[goalplayer[snum]].posz;
        if (!FAFcansee(x1,y1,z1-(48<<8),damysect,x2,y2,z2-(48<<8),sprite[Player[goalplayer[snum]].PlayerSprite].sectnum))
            goalplayer[snum] = snum;
    }

    // Pick a new target player if goal is dead or target is itself
    if (goalplayer[snum] == snum)
    {
        j = 0x7fffffff;
        for (i=connecthead; i>=0; i=connectpoint2[i])
            if (i != snum)
            {
                if (TEST(Player[i].Flags, PF_DEAD))
                    continue;

                x2 = Player[i].posx;
                y2 = Player[i].posy;
                z2 = Player[i].posz;

                if (!FAFcansee(x1,y1,z1-(48<<8),damysect,x2,y2,z2-(48<<8),sprite[Player[i].PlayerSprite].sectnum))
                    continue;

                dist = ksqrt((sprite[Player[i].PlayerSprite].x-x1)*(sprite[Player[i].PlayerSprite].x-x1)+(sprite[Player[i].PlayerSprite].y-y1)*(sprite[Player[i].PlayerSprite].y-y1));

                if (dist < j) { j = dist; goalplayer[snum] = i; }
            }
    }

    // Pick a weapon
    BOT_ChooseWeapon(p, u, syn);

    // x2,y2,z2 is the coordinates of the target sprite
    x2 = Player[goalplayer[snum]].posx;
    y2 = Player[goalplayer[snum]].posy;
    z2 = Player[goalplayer[snum]].posz;

    // If bot is dead, either barf or respawn
    if (TEST(p->Flags, PF_DEAD))
    {
        if (STD_RANDOM_RANGE(1000) > 990)
        {
            syn->bits |= (1<<SK_SPACE_BAR);  // Respawn
        }
        else
            syn->bits |= (1<<SK_SHOOT);    // Try to barf
    }

    // Need Health?
    if (u->Health < p->MaxHealth)
        BOT_UseInventory(p, INVENTORY_MEDKIT, syn);

    // Check the missile stat lists to see what's being fired and
    // take the appropriate action
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_MISSILE], j, nextj)
    {
        switch (sprite[j].picnum)
        {
        case FIREBALL: k = 0; break;
        case BOLT_THINMAN_R0:
            k = 0;
            syn->bits |= (1<<SK_JUMP); // Always jump when rockets being fired!
            break;
        default: k = 0; break;
        }
        if (k)
        {
            hitinfo.pos.x = sprite[j].x;
            hitinfo.pos.y = sprite[j].y;
            hitinfo.pos.z = sprite[j].z;
            for (l=0; l<=8; l++)
            {
                if (tmulscale11(hitinfo.pos.x-x1,hitinfo.pos.x-x1,hitinfo.pos.y-y1,hitinfo.pos.y-y1,(hitinfo.pos.z-z1)>>4,(hitinfo.pos.z-z1)>>4) < 3072)
                {
                    dx = sintable[(sprite[j].ang+512)&2047];
                    dy = sintable[sprite[j].ang&2047];
                    if ((x1-hitinfo.pos.x)*dy > (y1-hitinfo.pos.y)*dx) i = -k*512; else i = k*512;
                    syn->vel -= mulscale17(dy,i);
                    syn->svel += mulscale17(dx,i);
                }

                if (l < 7)
                {
                    hitinfo.pos.x += (mulscale14(sprite[j].xvel,sintable[(sprite[j].ang+512)&2047])<<2);
                    hitinfo.pos.y += (mulscale14(sprite[j].xvel,sintable[sprite[j].ang&2047])<<2);
                    hitinfo.pos.z += (sprite[j].zvel<<2);
                }
                else
                {
                    hitscan((vec3_t *)&sprite[j],sprite[j].sectnum,
                            mulscale14(sprite[j].xvel,sintable[(sprite[j].ang+512)&2047]),
                            mulscale14(sprite[j].xvel,sintable[sprite[j].ang&2047]),
                            (int)sprite[j].zvel,
                            &hitinfo,CLIPMASK1);
                }
            }
        }
    }


    if (!TEST(Player[goalplayer[snum]].Flags, PF_DEAD) && snum != goalplayer[snum] &&
        ((FAFcansee(x1,y1,z1-(24<<8),damysect,x2,y2,z2-(24<<8),sprite[Player[goalplayer[snum]].PlayerSprite].sectnum)) ||
         (FAFcansee(x1,y1,z1-(48<<8),damysect,x2,y2,z2-(48<<8),sprite[Player[goalplayer[snum]].PlayerSprite].sectnum))))
    {
        // Shoot how often by skill level
        short shootrnd=0;

        shootrnd = STD_RANDOM_RANGE(1000);

        if ((Skill == 0 && shootrnd > 990) ||
            (Skill == 1 && shootrnd > 550) ||
            (Skill == 2 && shootrnd > 350) ||
            (Skill == 3))
            syn->bits |= (1<<SK_SHOOT);
        else
            syn->bits &= ~(1<<SK_SHOOT);

        // Jump sometimes, to try to be evasive
        if (STD_RANDOM_RANGE(256) > 252)
            syn->bits |= (1<<SK_JUMP);

        // Make sure selected weapon is in range
        //ASSERT(p->WpnNum < MAX_WEAPONS);
        //ASSERT(Player[goalplayer[snum]].WpnNum < MAX_WEAPONS);

        // Only fire explosive type weaps if you are not too close to the target!
        if (u->WeaponNum == WPN_MICRO || u->WeaponNum == WPN_GRENADE || u->WeaponNum == WPN_RAIL)
        {
            vec3_t hit_pos = { x1, y1, z1-PLAYER_HEIGHT };
            hitscan(&hit_pos,damysect,sintable[(damyang+512)&2047],sintable[damyang&2047],
                    (100-p->horiz-p->horizoff)*32,&hitinfo,CLIPMASK1);
            if ((hitinfo.pos.x-x1)*(hitinfo.pos.x-x1)+(hitinfo.pos.y-y1)*(hitinfo.pos.y-y1) < 2560*2560) syn->bits &= ~(1<<SK_SHOOT);
        }

        // Get fighting distance based on you and your opponents current weapons
        fightdist = fdmatrix[p->WpnNum][Player[goalplayer[snum]].WpnNum];
        if (fightdist < 128) fightdist = 128;

        // Figure out your distance from the enemy target sprite
        dist = ksqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1)); if (dist == 0) dist = 1;
        daang = NORM_ANGLE(getangle(x2+(Player[goalplayer[snum]].xvect>>14)-x1,y2+(Player[goalplayer[snum]].yvect>>14)-y1));

        zang = 100-((z2-z1)*8)/dist;
        fightdist = max(fightdist,(klabs(z2-z1)>>4));

        hitinfo.pos.x = x2+((x1-x2)*fightdist/dist);
        hitinfo.pos.y = y2+((y1-y2)*fightdist/dist);
        syn->vel += (hitinfo.pos.x-x1)*2047/dist;
        syn->svel += (hitinfo.pos.y-y1)*2047/dist;

        //Strafe attack
        if (fightdist)
        {
            j = totalclock+snum*13468;
            i = sintable[(j<<6)&2047];
            i += sintable[((j+4245)<<5)&2047];
            i += sintable[((j+6745)<<4)&2047];
            i += sintable[((j+15685)<<3)&2047];
            dx = sintable[(sprite[Player[goalplayer[snum]].PlayerSprite].ang+512)&2047];
            dy = sintable[sprite[Player[goalplayer[snum]].PlayerSprite].ang&2047];
            if ((x1-x2)*dy > (y1-y2)*dx) i += 8192; else i -= 8192;
            syn->vel += ((sintable[(daang+1024)&2047]*i)>>17);
            syn->svel += ((sintable[(daang+512)&2047]*i)>>17);
        }

        // Make aiming and running speed suck by skill level
        if (Skill == 0)
        {
            daang = NORM_ANGLE((daang-256) + STD_RANDOM_RANGE(512));
            syn->vel -= syn->vel/2;
            syn->svel -= syn->svel/2;
        }
        else if (Skill == 1)
        {
            daang = NORM_ANGLE((daang-128) + STD_RANDOM_RANGE(256));
            syn->vel -= syn->vel/8;
            syn->svel -= syn->svel/8;
        }
        else if (Skill == 2)
            daang = NORM_ANGLE((daang-64) + STD_RANDOM_RANGE(128));

        // Below formula fails in certain cases
        //syn->angvel = min(max((((daang+1024-damyang)&2047)-1024)>>1,-MAXANGVEL),MAXANGVEL); //was 127
        p->pang = daang;
        syn->aimvel = min(max((zang-p->horiz)>>1,-PLAYER_HORIZ_MAX),PLAYER_HORIZ_MAX);
        // Sets type of aiming, auto aim for bots
        syn->bits |= (1<<SK_AUTO_AIM);
        return;
    }

    goalsect[snum] = -1;

#if 1
    if (goalsect[snum] < 0)
    {
        goalwall[snum] = -1;
        startsect = sprite[p->PlayerSprite].sectnum;
        endsect = sprite[Player[goalplayer[snum]].PlayerSprite].sectnum;

        clearbufbyte(dashow2dsector,(MAXSECTORS+7)>>3,0L);
        searchsect[0] = startsect;
        searchparent[0] = -1;
        dashow2dsector[startsect>>3] |= (1<<(startsect&7));
        for (splc=0,send=1; splc<send; splc++)
        {
            startwall = sector[searchsect[splc]].wallptr;
            endwall = startwall + sector[searchsect[splc]].wallnum;
            for (i=startwall,wal=&wall[startwall]; i<endwall; i++,wal++)
            {
                j = wal->nextsector; if (j < 0) continue;

                dx = ((wall[wal->point2].x+wal->x)>>1);
                dy = ((wall[wal->point2].y+wal->y)>>1);
                if ((getceilzofslope(j,dx,dy) > getflorzofslope(j,dx,dy)-(28<<8)) && ((sector[j].lotag < 15) || (sector[j].lotag > 22)))
                    continue;
                if (getflorzofslope(j,dx,dy) < getflorzofslope(searchsect[splc],dx,dy)-(72<<8))
                    continue;
                if ((dashow2dsector[j>>3]&(1<<(j&7))) == 0)
                {
                    dashow2dsector[j>>3] |= (1<<(j&7));
                    searchsect[send] = (short)j;
                    searchparent[send] = (short)splc;
                    send++;
                    if (j == endsect)
                    {
                        clearbufbyte(dashow2dsector,(MAXSECTORS+7)>>3,0L);
                        for (k=send-1; k>=0; k=searchparent[k])
                            dashow2dsector[searchsect[k]>>3] |= (1<<(searchsect[k]&7));

                        for (k=send-1; k>=0; k=searchparent[k])
                            if (!searchparent[k]) break;

                        goalsect[snum] = searchsect[k];
                        startwall = sector[goalsect[snum]].wallptr;
                        endwall = startwall+sector[goalsect[snum]].wallnum;
                        hitinfo.pos.x = hitinfo.pos.y = 0;
                        for (i=startwall; i<endwall; i++)
                        {
                            hitinfo.pos.x += wall[i].x;
                            hitinfo.pos.y += wall[i].y;
                        }
                        hitinfo.pos.x /= (endwall-startwall);
                        hitinfo.pos.y /= (endwall-startwall);

                        startwall = sector[startsect].wallptr;
                        endwall = startwall+sector[startsect].wallnum;
                        l = 0; k = startwall;
                        for (i=startwall; i<endwall; i++)
                        {
                            if (wall[i].nextsector != goalsect[snum]) continue;
                            dx = wall[wall[i].point2].x-wall[i].x;
                            dy = wall[wall[i].point2].y-wall[i].y;

                            //if (dx*(y1-wall[i].y) <= dy*(x1-wall[i].x))
                            //   if (dx*(y2-wall[i].y) >= dy*(x2-wall[i].x))
                            if ((hitinfo.pos.x-x1)*(wall[i].y-y1) <= (hitinfo.pos.y-y1)*(wall[i].x-x1))
                                if ((hitinfo.pos.x-x1)*(wall[wall[i].point2].y-y1) >= (hitinfo.pos.y-y1)*(wall[wall[i].point2].x-x1))
                                { k = i; break; }

                            dist = ksqrt(dx*dx+dy*dy);
                            if (dist > l) { l = dist; k = i; }
                        }
                        goalwall[snum] = k;
                        daang = ((getangle(wall[wall[k].point2].x-wall[k].x,wall[wall[k].point2].y-wall[k].y)+1536)&2047);
                        goalx[snum] = ((wall[k].x+wall[wall[k].point2].x)>>1)+(sintable[(daang+512)&2047]>>8);
                        goaly[snum] = ((wall[k].y+wall[wall[k].point2].y)>>1)+(sintable[daang&2047]>>8);
                        goalz[snum] = sector[goalsect[snum]].floorz-(32<<8);
                        break;
                    }
                }
            }

#if 0
            for (i=headspritesect[searchsect[splc]]; i>=0; i=nextspritesect[i])
                if (sprite[i].lotag == 7)
                {
                    j = sprite[sprite[i].owner].sectnum;
                    if ((dashow2dsector[j>>3]&(1<<(j&7))) == 0)
                    {
                        dashow2dsector[j>>3] |= (1<<(j&7));
                        searchsect[send] = (short)j;
                        searchparent[send] = (short)splc;
                        send++;
                        if (j == endsect)
                        {
                            clearbufbyte(dashow2dsector,(MAXSECTORS+7)>>3,0L);
                            for (k=send-1; k>=0; k=searchparent[k])
                                dashow2dsector[searchsect[k]>>3] |= (1<<(searchsect[k]&7));

                            for (k=send-1; k>=0; k=searchparent[k])
                                if (!searchparent[k]) break;

                            goalsect[snum] = searchsect[k];
                            startwall = sector[startsect].wallptr;
                            endwall = startwall+sector[startsect].wallnum;
                            l = 0; k = startwall;
                            for (i=startwall; i<endwall; i++)
                            {
                                dx = wall[wall[i].point2].x-wall[i].x;
                                dy = wall[wall[i].point2].y-wall[i].y;
                                dist = ksqrt(dx*dx+dy*dy);
                                if ((wall[i].nextsector == goalsect[snum]) && (dist > l))
                                { l = dist; k = i; }
                            }
                            goalwall[snum] = k;
                            daang = ((getangle(wall[wall[k].point2].x-wall[k].x,wall[wall[k].point2].y-wall[k].y)+1536)&2047);
                            goalx[snum] = ((wall[k].x+wall[wall[k].point2].x)>>1)+(sintable[(daang+512)&2047]>>8);
                            goaly[snum] = ((wall[k].y+wall[wall[k].point2].y)>>1)+(sintable[daang&2047]>>8);
                            goalz[snum] = sector[goalsect[snum]].floorz-(32<<8);
                            break;
                        }
                    }
                }
            if (goalwall[snum] >= 0) break;
#endif
        }
    }

    if ((goalsect[snum] < 0) || (goalwall[snum] < 0))
    {
        if (goalsprite[snum] < 0)
        {
            for (k=0; k<4; k++)
            {
                i = (rand()%numsectors);
                for (j=headspritesect[i]; j>=0; j=nextspritesect[j])
                {
                    if ((sprite[j].xrepeat <= 0) || (sprite[j].yrepeat <= 0)) continue;
                    if (getspritescore(snum,sprite[j].picnum) <= 0) continue;
                    if (FAFcansee(x1,y1,z1-(32<<8),damysect,sprite[j].x,sprite[j].y,sprite[j].z-(4<<8),i))
                    { goalx[snum] = sprite[j].x; goaly[snum] = sprite[j].y; goalz[snum] = sprite[j].z; goalsprite[snum] = j; break; }
                }
            }
        }
        x2 = goalx[snum];
        y2 = goaly[snum];
        dist = ksqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1)); if (!dist) return;
        daang = getangle(x2-x1,y2-y1);
        syn->vel += (x2-x1)*2047/dist;
        syn->svel += (y2-y1)*2047/dist;
        syn->angvel = min(max((((daang+1024-damyang)&2047)-1024)>>3,-MAXANGVEL),MAXANGVEL);
    }
    else
        goalsprite[snum] = -1;
#endif

    hitinfo.pos.x = p->posx; hitinfo.pos.y = p->posy; hitinfo.pos.z = p->posz; hitinfo.sect = p->cursectnum;
    i = clipmove(&hitinfo.pos,&hitinfo.sect,p->xvect,p->yvect,164L,4L<<8,4L<<8,CLIPMASK0);
    if (!i)
    {
        hitinfo.pos.x = p->posx; hitinfo.pos.y = p->posy; hitinfo.pos.z = p->posz+(24<<8); hitinfo.sect = p->cursectnum;
        i = clipmove(&hitinfo.pos,&hitinfo.sect,p->xvect,p->yvect,164L,4L<<8,4L<<8,CLIPMASK0);
    }
    if (i)
    {
        clipmovecount[snum]++;

        j = 0;
        if ((i&0xc000) == 32768)  //Hit a wall (49152 for sprite)
            if (wall[i&(MAXWALLS-1)].nextsector >= 0)
            {
                if (getflorzofslope(wall[i&(MAXWALLS-1)].nextsector,p->posx,p->posy) <= p->posz+(24<<8)) j |= 1;
                if (getceilzofslope(wall[i&(MAXWALLS-1)].nextsector,p->posx,p->posy) >= p->posz-(24<<8)) j |= 2;
            }
        if ((i&0xc000) == 49152) j = 1;
        // Jump
        if (j&1) if (clipmovecount[snum] == 4) syn->bits |= (1<<SK_JUMP);
        // Crawl
        if (j&2) syn->bits |= (1<<SK_CRAWL);

        //Strafe attack
        daang = getangle(x2-x1,y2-y1);
        if ((i&0xc000) == 32768)
            daang = getangle(wall[wall[i&(MAXWALLS-1)].point2].x-wall[i&(MAXWALLS-1)].x,wall[wall[i&(MAXWALLS-1)].point2].y-wall[i&(MAXWALLS-1)].y);
        j = totalclock+snum*13468;
        i = sintable[(j<<6)&2047];
        i += sintable[((j+4245)<<5)&2047];
        i += sintable[((j+6745)<<4)&2047];
        i += sintable[((j+15685)<<3)&2047];
        syn->vel += ((sintable[(daang+1024)&2047]*i)>>17);
        syn->svel += ((sintable[(daang+512)&2047]*i)>>17);

        // Try to Open
        if ((clipmovecount[snum]&31) == 2) syn->bits |= (1<<SK_OPERATE);
        // *TODO: In Duke, this is Kick, but I need to select sword then fire in SW
//        if ((clipmovecount[snum]&31) == 17) syn->bits |= (1<<22);
        if (clipmovecount[snum] > 32) { goalsect[snum] = -1; goalwall[snum] = -1; clipmovecount[snum] = 0; }

        goalsprite[snum] = -1;
    }
    else
        clipmovecount[snum] = 0;

    if ((goalsect[snum] >= 0) && (goalwall[snum] >= 0))
    {
        x2 = goalx[snum];
        y2 = goaly[snum];
        dist = ksqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1)); if (!dist) return;
        daang = getangle(x2-x1,y2-y1);
        if ((goalwall[snum] >= 0) && (dist < 4096))
            daang = ((getangle(wall[wall[goalwall[snum]].point2].x-wall[goalwall[snum]].x,wall[wall[goalwall[snum]].point2].y-wall[goalwall[snum]].y)+1536)&2047);
        syn->vel += (x2-x1)*2047/dist;
        syn->svel += (y2-y1)*2047/dist;
        syn->angvel = min(max((((daang+1024-damyang)&2047)-1024)>>3,-MAXANGVEL),MAXANGVEL);
    }


    /*
        // Use extended bot logic for navigation through level
        goalsect[snum] = -1;
        goalwall[snum] = -1;
        goalsprite[snum] = -1;

        // Go to a goal place
        if ((goalsect[snum] >= 0) && (goalwall[snum] >= 0))
        {
            x2 = goalx[snum];
            y2 = goaly[snum];
            dist = ksqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1)); if (!dist) return;
            daang = getangle(x2-x1,y2-y1);
            if ((goalwall[snum] >= 0) && (dist < 4096))
                daang = ((getangle(wall[wall[goalwall[snum]].point2].x-wall[goalwall[snum]].x,wall[wall[goalwall[snum]].point2].y-wall[goalwall[snum]].y)+1536)&2047);
            syn->vel += (x2-x1)*2047/dist;
            syn->svel += (y2-y1)*2047/dist;
            syn->angvel = min(max((((daang+1024-damyang)&2047)-1024)>>3,-MAXANGVEL),MAXANGVEL);
        }
    */
}

