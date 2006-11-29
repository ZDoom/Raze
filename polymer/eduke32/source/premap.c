//-------------------------------------------------------------------------
/*
Copyright (C) 2005 - EDuke32 team

This file is part of EDuke32

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#include "duke3d.h"
#include "osd.h"

extern char pow2char[];

extern char everyothertime;
static short which_palookup = 9;
char numl, useprecache = 1;
short spritecache[MAXTILES][3];

static char precachehightile[2][MAXTILES>>3];
static int  precachecount;

static void tloadtile(short tilenume, char type)
{
    if ((picanm[tilenume]&63) > 0)
    {
        int i,j;

        if ((picanm[tilenume]&192)==192)
        {
            i = tilenume - (picanm[tilenume]&63);
            j = tilenume;
        }
        else
        {
            i = tilenume;
            j = tilenume + (picanm[tilenume]&63);
        }
        for (;i<=j;i++)
        {
            if (!(gotpic[i>>3] & pow2char[i&7])) precachecount++;
            gotpic[i>>3] |= pow2char[i&7];
            precachehightile[(unsigned char)type][i>>3] |= pow2char[i&7];
        }
    }
    else
    {
        if (!(gotpic[tilenume>>3] & pow2char[tilenume&7])) precachecount++;
        gotpic[tilenume>>3] |= pow2char[tilenume&7];
        precachehightile[(unsigned char)type][tilenume>>3] |= pow2char[tilenume&7];
    }
}

void cachespritenum(short i)
{
    char maxc;
    short j;

    if (ud.monsters_off && badguy(&sprite[i])) return;

    maxc = 1;

    if (spritecache[PN][0] == PN)
        for (j = PN; j <= spritecache[PN][1]; j++)
            tloadtile(j,1);

    switch (dynamictostatic[PN])
    {
    case HYDRENT__STATIC:
        tloadtile(BROKEFIREHYDRENT,1);
        for (j = TOILETWATER; j < (TOILETWATER+4); j++) tloadtile(j,1);
        break;
    case TOILET__STATIC:
        tloadtile(TOILETBROKE,1);
        for (j = TOILETWATER; j < (TOILETWATER+4); j++) tloadtile(j,1);
        break;
    case STALL__STATIC:
        tloadtile(STALLBROKE,1);
        for (j = TOILETWATER; j < (TOILETWATER+4); j++) tloadtile(j,1);
        break;
    case RUBBERCAN__STATIC:
        maxc = 2;
        break;
    case TOILETWATER__STATIC:
        maxc = 4;
        break;
    case FEMPIC1__STATIC:
        maxc = 44;
        break;
    case LIZTROOP__STATIC:
    case LIZTROOPRUNNING__STATIC:
    case LIZTROOPSHOOT__STATIC:
    case LIZTROOPJETPACK__STATIC:
    case LIZTROOPONTOILET__STATIC:
    case LIZTROOPDUCKING__STATIC:
        for (j = LIZTROOP; j < (LIZTROOP+72); j++) tloadtile(j,1);
        for (j=HEADJIB1;j<LEGJIB1+3;j++) tloadtile(j,1);
        maxc = 0;
        break;
    case WOODENHORSE__STATIC:
        maxc = 5;
        for (j = HORSEONSIDE; j < (HORSEONSIDE+4); j++) tloadtile(j,1);
        break;
    case NEWBEAST__STATIC:
    case NEWBEASTSTAYPUT__STATIC:
        maxc = 90;
        break;
    case BOSS1__STATIC:
    case BOSS2__STATIC:
    case BOSS3__STATIC:
        maxc = 30;
        break;
    case OCTABRAIN__STATIC:
    case OCTABRAINSTAYPUT__STATIC:
    case COMMANDER__STATIC:
    case COMMANDERSTAYPUT__STATIC:
        maxc = 38;
        break;
    case RECON__STATIC:
        maxc = 13;
        break;
    case PIGCOP__STATIC:
    case PIGCOPDIVE__STATIC:
        maxc = 61;
        break;
    case SHARK__STATIC:
        maxc = 30;
        break;
    case LIZMAN__STATIC:
    case LIZMANSPITTING__STATIC:
    case LIZMANFEEDING__STATIC:
    case LIZMANJUMP__STATIC:
        for (j=LIZMANHEAD1;j<LIZMANLEG1+3;j++) tloadtile(j,1);
        maxc = 80;
        break;
    case APLAYER__STATIC:
        maxc = 0;
        if (ud.multimode > 1)
        {
            maxc = 5;
            for (j = 1420;j < 1420+106; j++) tloadtile(j,1);
        }
        break;
    case ATOMICHEALTH__STATIC:
        maxc = 14;
        break;
    case DRONE__STATIC:
        maxc = 10;
        break;
    case EXPLODINGBARREL__STATIC:
    case SEENINE__STATIC:
    case OOZFILTER__STATIC:
        maxc = 3;
        break;
    case NUKEBARREL__STATIC:
    case CAMERA1__STATIC:
        maxc = 5;
        break;
        // caching of HUD sprites for weapons that may be in the level
    case CHAINGUNSPRITE__STATIC:
        for (j=CHAINGUN; j<=CHAINGUN+7; j++) tloadtile(j,1);
        break;
    case RPGSPRITE__STATIC:
        for (j=RPGGUN; j<=RPGGUN+2; j++) tloadtile(j,1);
        break;
    case FREEZESPRITE__STATIC:
        for (j=FREEZE; j<=FREEZE+5; j++) tloadtile(j,1);
        break;
    case GROWSPRITEICON__STATIC:
    case SHRINKERSPRITE__STATIC:
        for (j=SHRINKER-2; j<=SHRINKER+5; j++) tloadtile(j,1);
        break;
    case HBOMBAMMO__STATIC:
    case HEAVYHBOMB__STATIC:
        for (j=HANDREMOTE; j<=HANDREMOTE+5; j++) tloadtile(j,1);
        break;
    case TRIPBOMBSPRITE__STATIC:
        for (j=HANDHOLDINGLASER; j<=HANDHOLDINGLASER+4; j++) tloadtile(j,1);
        break;
    case SHOTGUNSPRITE__STATIC:
        tloadtile(SHOTGUNSHELL,1);
        for (j=SHOTGUN; j<=SHOTGUN+6; j++) tloadtile(j,1);
        break;
    case DEVISTATORSPRITE__STATIC:
        for (j=DEVISTATOR; j<=DEVISTATOR+1; j++) tloadtile(j,1);
        break;

    }

    for (j = PN; j < (PN+maxc); j++) tloadtile(j,1);
}

void cachegoodsprites(void)
{
    short i,j;

    for (i=0;i<MAXTILES;i++)
    {
        if (spriteflags[i] & SPRITE_FLAG_PROJECTILE)
            tloadtile(i,1);
        if (spritecache[i][0] == i && spritecache[i][2])
            for (j = i; j <= spritecache[i][1]; j++)
                tloadtile(j,1);
    }
    tloadtile(BOTTOMSTATUSBAR,1);
    if (ud.multimode > 1)
        tloadtile(FRAGBAR,1);

    tloadtile(VIEWSCREEN,1);

    for (i=STARTALPHANUM;i<ENDALPHANUM+1;i++) tloadtile(i,1);
    for (i=BIGALPHANUM; i<BIGALPHANUM+82; i++) tloadtile(i,1);
    for (i=MINIFONT;i<MINIFONT+63;i++) tloadtile(i,1);

    for (i=FOOTPRINTS;i<FOOTPRINTS+3;i++) tloadtile(i,1);

    for (i = BURNING; i < BURNING+14; i++) tloadtile(i,1);
    for (i = BURNING2; i < BURNING2+14; i++) tloadtile(i,1);

    for (i = CRACKKNUCKLES; i < CRACKKNUCKLES+4; i++) tloadtile(i,1);

    for (i = FIRSTGUN; i < FIRSTGUN+3 ; i++) tloadtile(i,1);
    for (i = FIRSTGUNRELOAD; i < FIRSTGUNRELOAD+8 ; i++) tloadtile(i,1);

    for (i = EXPLOSION2; i < EXPLOSION2+21 ; i++) tloadtile(i,1);

    tloadtile(BULLETHOLE,1);
    for (i = SMALLSMOKE; i < (SMALLSMOKE+4); i++) tloadtile(i,1);

    for (i = JIBS1; i < (JIBS5+5); i++) tloadtile(i,1);
    for (i = SCRAP1; i < (SCRAP1+19); i++) tloadtile(i,1);

    for (i=RPG; i<RPG+7; i++) tloadtile(i,1);
    for (i=FREEZEBLAST; i<FREEZEBLAST+3; i++) tloadtile(i,1);
    for (i=SHRINKSPARK; i<SHRINKSPARK+4; i++) tloadtile(i,1);
    for (i=GROWSPARK; i<GROWSPARK+4; i++) tloadtile(i,1);
    for (i=SHRINKEREXPLOSION; i<SHRINKEREXPLOSION+4; i++) tloadtile(i,1);
    for (i=MORTER; i<MORTER+4; i++) tloadtile(i,4);
}

char getsound(unsigned short num)
{
    short fp;
    long   l;

    if (num >= NUM_SOUNDS || SoundToggle == 0) return 0;
    if (FXDevice < 0) return 0;

    if (!sounds[num][0]) return 0;
    fp = kopen4load(sounds[num],loadfromgrouponly);
    if (fp == -1) return 0;

    l = kfilelength(fp);
    soundsiz[num] = l;

    if ((ud.level_number == 0 && ud.volume_number == 0 && (num == 189 || num == 232 || num == 99 || num == 233 || num == 17)) ||
            (l < 12288))
    {
        Sound[num].lock = 199;
        allocache((long *)&Sound[num].ptr,l,(char *)&Sound[num].lock);
        if (Sound[num].ptr != NULL)
            kread(fp, Sound[num].ptr , l);
    }
    kclose(fp);
    return 1;
}

void precachenecessarysounds(void)
{
    short i, j;

    if (FXDevice < 0) return;
    j = 0;

    for (i=0;i<NUM_SOUNDS;i++)
        if (Sound[i].ptr == 0)
        {
            j++;
            if ((j&7) == 0)
            {
                handleevents();
                getpackets();
            }
            getsound(i);
        }
}

void cacheit(void)
{
    long i,j,k, pc=0;
    long tc;
    unsigned long starttime, endtime;

    if (ud.recstat == 2)
        return;

    starttime = getticks();

    precachenecessarysounds();

    cachegoodsprites();

    for (i=0;i<numwalls;i++)
    {
        tloadtile(wall[i].picnum, 0);

        if (wall[i].overpicnum >= 0)
        {
            tloadtile(wall[i].overpicnum, 0);
        }
    }

    for (i=0;i<numsectors;i++)
    {
        tloadtile(sector[i].floorpicnum, 0);
        tloadtile(sector[i].ceilingpicnum, 0);
        if (sector[i].ceilingpicnum == LA)  // JBF 20040509: if( waloff[sector[i].ceilingpicnum] == LA) WTF??!??!?!?
        {
            tloadtile(LA+1, 0);
            tloadtile(LA+2, 0);
        }

        j = headspritesect[i];
        while (j >= 0)
        {
            if (sprite[j].xrepeat != 0 && sprite[j].yrepeat != 0 && (sprite[j].cstat&32768) == 0)
                cachespritenum(j);
            j = nextspritesect[j];
        }
    }

    tc = totalclock;
    j = 0;

    for (i=0;i<MAXTILES;i++)
    {
        if (!(i&7) && !gotpic[i>>3])
        {
            i+=7;
            continue;
        }
        if (gotpic[i>>3] & pow2char[i&7])
        {
            if (waloff[i] == 0)
                loadtile((short)i);

#if defined(POLYMOST) && defined(USE_OPENGL)
            if (useprecache && !KB_KeyPressed(sc_Space))
            {
                if (precachehightile[0][i>>3] & pow2char[i&7])
                    for (k=0; k<MAXPALOOKUPS; k++)
                        polymost_precache(i,k,0);

                if (precachehightile[1][i>>3] & pow2char[i&7])
                    for (k=0; k<MAXPALOOKUPS; k++)
                        polymost_precache(i,k,1);
            }
#endif
            j++;
            pc++;
        }
        else continue;

        if ((j&7) == 0)
        {
            handleevents();
            getpackets();
        }
        if (totalclock - tc > TICRATE/4)
        {
            sprintf(tempbuf,"Loading textures ... %ld%%\n",min(100,100*pc/precachecount));
            dofrontscreens(tempbuf);
            tc = totalclock;
        }
    }

    clearbufbyte(gotpic,sizeof(gotpic),0L);

    endtime = getticks();
    OSD_Printf("Cache time: %dms\n", endtime-starttime);
}

void xyzmirror(short i,short wn)
{
    //if (waloff[wn] == 0) loadtile(wn);
    setviewtotile(wn,tilesizy[wn],tilesizx[wn]);

    drawrooms(SX,SY,SZ,SA,100+sprite[i].shade,SECT);
    display_mirror = 1;
    animatesprites(SX,SY,SA,65536L);
    display_mirror = 0;
    drawmasks();

    setviewback();
    squarerotatetile(wn);
    invalidatetile(wn,-1,255);
}

void vscrn(void)
{
    long i, j, ss, x1, x2, y1, y2;

    if (ud.screen_size < 0) ud.screen_size = 0;
    else if (ud.screen_size > 63) ud.screen_size = 64;

    if (ud.screen_size == 0) flushperms();

    ss = max(ud.screen_size-8,0);

    x1 = scale(ss,xdim,160);
    x2 = xdim-x1;

    y1 = ss;
    y2 = 200;
    if (ud.screen_size > 0 && (gametype_flags[ud.coop]&GAMETYPE_FLAG_FRAGBAR) && ud.multimode > 1)
    {
        j = 0;
        for (i=connecthead;i>=0;i=connectpoint2[i])
            if (i > j) j = i;

        if (j >= 1) y1 += 8;
        if (j >= 4) y1 += 8;
        if (j >= 8) y1 += 8;
        if (j >= 12) y1 += 8;
    }

    if (ud.screen_size >= 8 && !(ud.screen_size == 8 && ud.statusbarmode && bpp > 8)) y2 -= (ss+scale(tilesizy[BOTTOMSTATUSBAR],ud.statusbarscale,100));

    y1 = scale(y1,ydim,200);
    y2 = scale(y2,ydim,200);

    setview(x1,y1,x2-1,y2-1);

    pub = NUMPAGES;
    pus = NUMPAGES;
}

void pickrandomspot(short snum)
{
    struct player_struct *p;
    short i=0,j,k;
    unsigned long dist,pdist = -1;

    p = &ps[snum];

    if (ud.multimode > 1 && !(gametype_flags[ud.coop] & GAMETYPE_FLAG_FIXEDRESPAWN))
    {
        if (gametype_flags[ud.coop] & GAMETYPE_FLAG_TDMSPAWN)
        {
            for (j=0;j<ud.multimode;j++)
            {
                if (j != snum && ps[j].team == ps[snum].team && sprite[ps[j].i].extra > 0)
                {
                    for (k=0;k<numplayersprites;k++)
                    {
                        dist = FindDistance2D(ps[j].posx-po[k].ox,ps[j].posy-po[k].oy);
                        if (dist < pdist)
                            i = k, pdist = dist;
                    }
                    break;
                }
            }
        }
        else i = TRAND%numplayersprites;
    }
    else i = snum;

    p->bobposx = p->oposx = p->posx = po[i].ox;
    p->bobposy = p->oposy = p->posy = po[i].oy;
    p->oposz = p->posz = po[i].oz;
    p->ang = po[i].oa;
    p->cursectnum = po[i].os;
}

void resetplayerstats(short snum)
{
    struct player_struct *p;

    p = &ps[snum];

    ud.show_help        = 0;
    ud.showallmap       = 0;
    p->dead_flag        = 0;
    p->wackedbyactor    = -1;
    p->falling_counter  = 0;
    p->quick_kick       = 0;
    p->subweapon        = 0;
    p->last_full_weapon = 0;
    p->ftq              = 0;
    p->fta              = 0;
    p->tipincs          = 0;
    p->buttonpalette    = 0;
    p->actorsqu         =-1;
    p->invdisptime      = 0;
    p->refresh_inventory= 0;
    p->last_pissed_time = 0;
    p->holster_weapon   = 0;
    p->pycount          = 0;
    p->pyoff            = 0;
    p->opyoff           = 0;
    p->loogcnt          = 0;
    p->angvel           = 0;
    p->weapon_sway      = 0;
    //    p->select_dir       = 0;
    p->extra_extra8     = 0;
    p->show_empty_weapon= 0;
    p->dummyplayersprite=-1;
    p->crack_time       = 0;
    p->hbomb_hold_delay = 0;
    p->transporter_hold = 0;
    p->wantweaponfire  = -1;
    p->hurt_delay       = 0;
    p->footprintcount   = 0;
    p->footprintpal     = 0;
    p->footprintshade   = 0;
    p->jumping_toggle   = 0;
    p->ohoriz = p->horiz= 140;
    p->horizoff         = 0;
    p->bobcounter       = 0;
    p->on_ground        = 0;
    p->player_par       = 0;
    p->return_to_center = 9;
    p->airleft          = 15*26;
    p->rapid_fire_hold  = 0;
    p->toggle_key_flag  = 0;
    p->access_spritenum = -1;
    if (ud.multimode > 1 && (gametype_flags[ud.coop] & GAMETYPE_FLAG_ACCESSATSTART))
        p->got_access = 7;
    else p->got_access      = 0;
    p->random_club_frame= 0;
    pus = 1;
    p->on_warping_sector = 0;
    p->spritebridge      = 0;
    p->sbs          = 0;
    p->palette = (char *) &palette[0];

    if (p->steroids_amount < 400)
    {
        p->steroids_amount = 0;
        p->inven_icon = 0;
    }
    p->heat_on =            0;
    p->jetpack_on =         0;
    p->holoduke_on =       -1;

    p->look_ang          = 512 - ((ud.level_number&1)<<10);

    p->rotscrnang        = 0;
    p->orotscrnang       = 1;   // JBF 20031220
    p->newowner          =-1;
    p->jumping_counter   = 0;
    p->hard_landing      = 0;
    p->posxv             = 0;
    p->posyv             = 0;
    p->poszv             = 0;
    fricxv            = 0;
    fricyv            = 0;
    p->somethingonplayer =-1;
    p->one_eighty_count  = 0;
    p->cheat_phase       = 0;

    p->on_crane          = -1;

    if ((aplWeaponWorksLike[p->curr_weapon][snum] == PISTOL_WEAPON) &&
            (aplWeaponReload[p->curr_weapon][snum] > aplWeaponTotalTime[p->curr_weapon][snum]))
        p->kickback_pic  = aplWeaponTotalTime[p->curr_weapon][snum]+1;
    else p->kickback_pic = 0;

    p->weapon_pos        = 6;
    p->walking_snd_toggle= 0;
    p->weapon_ang        = 0;

    p->knuckle_incs      = 1;
    p->fist_incs = 0;
    p->knee_incs         = 0;
    p->jetpack_on        = 0;
    p->reloading        = 0;

    p->movement_lock     = 0;

    setpal(p);
    OnEvent(EVENT_RESETPLAYER, p->i, snum, -1);
}

void resetweapons(short snum)
{
    short  weapon;
    struct player_struct *p;

    p = &ps[snum];

    for (weapon = PISTOL_WEAPON; weapon < MAX_WEAPONS; weapon++)
        p->gotweapon[weapon] = 0;
    for (weapon = PISTOL_WEAPON; weapon < MAX_WEAPONS; weapon++)
        p->ammo_amount[weapon] = 0;

    p->weapon_pos = 6;
    p->kickback_pic = 5;
    p->curr_weapon = PISTOL_WEAPON;
    p->gotweapon[PISTOL_WEAPON] = 1;
    p->gotweapon[KNEE_WEAPON] = 1;
    p->ammo_amount[PISTOL_WEAPON] = 48;
    p->gotweapon[HANDREMOTE_WEAPON] = 1;
    p->last_weapon = -1;

    p->show_empty_weapon= 0;
    p->last_pissed_time = 0;
    p->holster_weapon = 0;
    OnEvent(EVENT_RESETWEAPONS, p->i, snum, -1);
}

void resetinventory(short snum)
{
    struct player_struct *p;

    p = &ps[snum];

    p->inven_icon       = 0;
    p->boot_amount = 0;
    p->scuba_on =           0;
    p->scuba_amount =         0;
    p->heat_amount        = 0;
    p->heat_on = 0;
    p->jetpack_on =         0;
    p->jetpack_amount =       0;
    p->shield_amount =      max_armour_amount;
    p->holoduke_on = -1;
    p->holoduke_amount =    0;
    p->firstaid_amount = 0;
    p->steroids_amount = 0;
    p->inven_icon = 0;
    OnEvent(EVENT_RESETINVENTORY, p->i, snum, -1);
}

void resetprestat(short snum,char g)
{
    struct player_struct *p;
    short i;

    p = &ps[snum];

    spriteqloc = 0;
    for (i=0;i<spriteqamount;i++) spriteq[i] = -1;

    p->hbomb_on          = 0;
    p->cheat_phase       = 0;
    p->pals_time         = 0;
    p->toggle_key_flag   = 0;
    p->secret_rooms      = 0;
    p->max_secret_rooms  = 0;
    p->actors_killed     = 0;
    p->max_actors_killed = 0;
    p->lastrandomspot = 0;
    p->weapon_pos = 6;
    p->kickback_pic = 5;
    p->last_weapon = -1;
    p->weapreccnt = 0;
    p->interface_toggle_flag = 0;
    p->show_empty_weapon= 0;
    p->holster_weapon = 0;
    p->last_pissed_time = 0;

    p->one_parallax_sectnum = -1;
    p->visibility = ud.const_visibility;

    screenpeek              = myconnectindex;
    numanimwalls            = 0;
    numcyclers              = 0;
    animatecnt              = 0;
    parallaxtype            = 0;
    randomseed              = 17L;
    ud.pause_on             = 0;
    ud.camerasprite         =-1;
    ud.eog                  = 0;
    tempwallptr             = 0;
    camsprite               =-1;
    earthquaketime          = 0;

    numinterpolations = 0;
    startofdynamicinterpolations = 0;

    if (((g&MODE_EOL) != MODE_EOL && numplayers < 2) || (!(gametype_flags[ud.coop]&GAMETYPE_FLAG_PRESERVEINVENTORYDEATH) && numplayers > 1))
    {
        resetweapons(snum);
        resetinventory(snum);
    }
    else if (p->curr_weapon == HANDREMOTE_WEAPON)
    {
        p->ammo_amount[HANDBOMB_WEAPON]++;
        p->curr_weapon = HANDBOMB_WEAPON;
    }

    p->timebeforeexit   = 0;
    p->customexitsound  = 0;

}

void setupbackdrop(short sky)
{
    short i;

    for (i=0;i<MAXPSKYTILES;i++) pskyoff[i]=0;

    if (parallaxyscale != 65536L)
        parallaxyscale = 32768;

    switch (dynamictostatic[sky])
    {
    case CLOUDYOCEAN__STATIC:
        parallaxyscale = 65536L;
        break;
    case MOONSKY1__STATIC :
        pskyoff[6]=1;
        pskyoff[1]=2;
        pskyoff[4]=2;
        pskyoff[2]=3;
        break;
    case BIGORBIT1__STATIC: // orbit
        pskyoff[5]=1;
        pskyoff[6]=2;
        pskyoff[7]=3;
        pskyoff[2]=4;
        break;
    case LA__STATIC:
        parallaxyscale = 16384+1024;
        pskyoff[0]=1;
        pskyoff[1]=2;
        pskyoff[2]=1;
        pskyoff[3]=3;
        pskyoff[4]=4;
        pskyoff[5]=0;
        pskyoff[6]=2;
        pskyoff[7]=3;
        break;
    }

    pskybits=3;
}

void prelevel(char g)
{
    short i, nexti, j, startwall, endwall, lotaglist;
    short lotags[65];
    int switchpicnum;


    clearbufbyte(show2dsector,sizeof(show2dsector),0L);
    clearbufbyte(show2dwall,sizeof(show2dwall),0L);
    clearbufbyte(show2dsprite,sizeof(show2dsprite),0L);

    resetprestat(0,g);
    numclouds = 0;

    for (i=0;i<numsectors;i++)
    {
        sector[i].extra = 256;

        switch (sector[i].lotag)
        {
        case 20:
        case 22:
            if (sector[i].floorz > sector[i].ceilingz)
                sector[i].lotag |= 32768;
            continue;
        }

        if (sector[i].ceilingstat&1)
        {
            if (waloff[sector[i].ceilingpicnum] == 0)
            {
                if (sector[i].ceilingpicnum == LA)
                    for (j=0;j<5;j++)
                        tloadtile(sector[i].ceilingpicnum+j, 0);
            }
            setupbackdrop(sector[i].ceilingpicnum);

            if (sector[i].ceilingpicnum == CLOUDYSKIES && numclouds < 127)
                clouds[numclouds++] = i;

            if (ps[0].one_parallax_sectnum == -1)
                ps[0].one_parallax_sectnum = i;
        }

        if (sector[i].lotag == 32767) //Found a secret room
        {
            ps[0].max_secret_rooms++;
            continue;
        }

        if (sector[i].lotag == -1)
        {
            ps[0].exitx = wall[sector[i].wallptr].x;
            ps[0].exity = wall[sector[i].wallptr].y;
            continue;
        }
    }

    i = headspritestat[0];
    while (i >= 0)
    {
        nexti = nextspritestat[i];
        ResetActorGameVars(i);
        LoadActor(i, -1, -1);
        if (sprite[i].lotag == -1 && (sprite[i].cstat&16))
        {
            ps[0].exitx = SX;
            ps[0].exity = SY;
        }
        else switch (dynamictostatic[PN])
            {
            case GPSPEED__STATIC:
                sector[SECT].extra = SLT;
                deletesprite(i);
                break;

            case CYCLER__STATIC:
                if (numcyclers >= MAXCYCLERS)
                {
                    Bsprintf(tempbuf,"\nToo many cycling sectors (%d max).",MAXCYCLERS);
                    gameexit(tempbuf);
                }
                cyclers[numcyclers][0] = SECT;
                cyclers[numcyclers][1] = SLT;
                cyclers[numcyclers][2] = SS;
                cyclers[numcyclers][3] = sector[SECT].floorshade;
                cyclers[numcyclers][4] = SHT;
                cyclers[numcyclers][5] = (SA == 1536);
                numcyclers++;
                deletesprite(i);
                break;

            case SECTOREFFECTOR__STATIC:
            case ACTIVATOR__STATIC:
            case TOUCHPLATE__STATIC:
            case ACTIVATORLOCKED__STATIC:
            case MUSICANDSFX__STATIC:
            case LOCATORS__STATIC:
            case MASTERSWITCH__STATIC:
            case RESPAWN__STATIC:
                sprite[i].cstat = 0;
                break;
            }
        i = nexti;
    }

    for (i=0;i < MAXSPRITES;i++)
    {
        if (sprite[i].statnum < MAXSTATUS)
        {
            if (PN == SECTOREFFECTOR && SLT == 14)
                continue;
            spawn(-1,i);
        }
    }

    for (i=0;i < MAXSPRITES;i++)
        if (sprite[i].statnum < MAXSTATUS)
        {
            if (PN == SECTOREFFECTOR && SLT == 14)
                spawn(-1,i);
        }

    lotaglist = 0;

    i = headspritestat[0];
    while (i >= 0)
    {
        switch (dynamictostatic[PN-1])
        {
        case DIPSWITCH__STATIC:
        case DIPSWITCH2__STATIC:
        case PULLSWITCH__STATIC:
        case HANDSWITCH__STATIC:
        case SLOTDOOR__STATIC:
        case LIGHTSWITCH__STATIC:
        case SPACELIGHTSWITCH__STATIC:
        case SPACEDOORSWITCH__STATIC:
        case FRANKENSTINESWITCH__STATIC:
        case LIGHTSWITCH2__STATIC:
        case POWERSWITCH1__STATIC:
        case LOCKSWITCH1__STATIC:
        case POWERSWITCH2__STATIC:
            for (j=0;j<lotaglist;j++)
                if (SLT == lotags[j])
                    break;

            if (j == lotaglist)
            {
                lotags[lotaglist] = SLT;
                lotaglist++;
                if (lotaglist > 64)
                    gameexit("\nToo many switches (64 max).");

                j = headspritestat[3];
                while (j >= 0)
                {
                    if (sprite[j].lotag == 12 && sprite[j].hitag == SLT)
                        hittype[j].temp_data[0] = 1;
                    j = nextspritestat[j];
                }
            }
            break;
        }
        i = nextspritestat[i];
    }

    mirrorcnt = 0;

    for (i = 0; i < numwalls; i++)
    {
        walltype *wal;
        wal = &wall[i];

        if (wal->overpicnum == MIRROR && (wal->cstat&32) != 0)
        {
            j = wal->nextsector;

            if (mirrorcnt > 63)
                gameexit("\nToo many mirrors (64 max.)");
            if ((j >= 0) && sector[j].ceilingpicnum != MIRROR)
            {
                sector[j].ceilingpicnum = MIRROR;
                sector[j].floorpicnum = MIRROR;
                mirrorwall[mirrorcnt] = i;
                mirrorsector[mirrorcnt] = j;
                mirrorcnt++;
                continue;
            }
        }

        if (numanimwalls >= MAXANIMWALLS)
        {
            Bsprintf(tempbuf,"\nToo many 'anim' walls (%d max).",MAXANIMWALLS);
            gameexit(tempbuf);
        }

        animwall[numanimwalls].tag = 0;
        animwall[numanimwalls].wallnum = 0;
        switchpicnum = wal->overpicnum;
        if ((wal->overpicnum > W_FORCEFIELD)&&(wal->overpicnum <= W_FORCEFIELD+2))
        {
            switchpicnum = W_FORCEFIELD;
        }
        switch (dynamictostatic[switchpicnum])
        {
        case FANSHADOW__STATIC:
        case FANSPRITE__STATIC:
            wall->cstat |= 65;
            animwall[numanimwalls].wallnum = i;
            numanimwalls++;
            break;

        case W_FORCEFIELD__STATIC:
            if (wal->overpicnum==W_FORCEFIELD__STATIC)
                for (j=0;j<3;j++)
                    tloadtile(W_FORCEFIELD+j, 0);
            if (wal->shade > 31)
                wal->cstat = 0;
            else wal->cstat |= 85+256;


            if (wal->lotag && wal->nextwall >= 0)
                wall[wal->nextwall].lotag =
                    wal->lotag;

        case BIGFORCE__STATIC:

            animwall[numanimwalls].wallnum = i;
            numanimwalls++;

            continue;
        }

        wal->extra = -1;

        switch (dynamictostatic[wal->picnum])
        {
        case WATERTILE2__STATIC:
            for (j=0;j<3;j++)
                tloadtile(wal->picnum+j, 0);
            break;

        case TECHLIGHT2__STATIC:
        case TECHLIGHT4__STATIC:
            tloadtile(wal->picnum, 0);
            break;
        case W_TECHWALL1__STATIC:
        case W_TECHWALL2__STATIC:
        case W_TECHWALL3__STATIC:
        case W_TECHWALL4__STATIC:
            animwall[numanimwalls].wallnum = i;
            //                animwall[numanimwalls].tag = -1;
            numanimwalls++;
            break;
        case SCREENBREAK6__STATIC:
        case SCREENBREAK7__STATIC:
        case SCREENBREAK8__STATIC:
            for (j=SCREENBREAK6;j<SCREENBREAK9;j++)
                tloadtile(j, 0);
            animwall[numanimwalls].wallnum = i;
            animwall[numanimwalls].tag = -1;
            numanimwalls++;
            break;

        case FEMPIC1__STATIC:
        case FEMPIC2__STATIC:
        case FEMPIC3__STATIC:

            wal->extra = wal->picnum;
            animwall[numanimwalls].tag = -1;
            if (ud.lockout)
            {
                if (wal->picnum == FEMPIC1)
                    wal->picnum = BLANKSCREEN;
                else wal->picnum = SCREENBREAK6;
            }

            animwall[numanimwalls].wallnum = i;
            animwall[numanimwalls].tag = wal->picnum;
            numanimwalls++;
            break;

        case SCREENBREAK1__STATIC:
        case SCREENBREAK2__STATIC:
        case SCREENBREAK3__STATIC:
        case SCREENBREAK4__STATIC:
        case SCREENBREAK5__STATIC:

        case SCREENBREAK9__STATIC:
        case SCREENBREAK10__STATIC:
        case SCREENBREAK11__STATIC:
        case SCREENBREAK12__STATIC:
        case SCREENBREAK13__STATIC:
        case SCREENBREAK14__STATIC:
        case SCREENBREAK15__STATIC:
        case SCREENBREAK16__STATIC:
        case SCREENBREAK17__STATIC:
        case SCREENBREAK18__STATIC:
        case SCREENBREAK19__STATIC:

            animwall[numanimwalls].wallnum = i;
            animwall[numanimwalls].tag = wal->picnum;
            numanimwalls++;
            break;
        }
    }

    //Invalidate textures in sector behind mirror
    for (i=0;i<mirrorcnt;i++)
    {
        startwall = sector[mirrorsector[i]].wallptr;
        endwall = startwall + sector[mirrorsector[i]].wallnum;
        for (j=startwall;j<endwall;j++)
        {
            wall[j].picnum = MIRROR;
            wall[j].overpicnum = MIRROR;
        }
    }
}

void newgame(char vn,char ln,char sk)
{
    struct player_struct *p = &ps[0];
    short i;

    if (globalskillsound >= 0 && FXDevice >= 0 && SoundToggle)
        while (issoundplaying(-1,globalskillsound))
        {
            handleevents();
            getpackets();
        }
    else
    {
        handleevents();
        getpackets();
    }
    globalskillsound = -1;

    waitforeverybody();
    ready2send = 0;

    if (ud.m_recstat != 2 && ud.last_level >= 0 && ud.multimode > 1 && (ud.coop&GAMETYPE_FLAG_SCORESHEET))
        dobonus(1);

    if (ln == 0 && vn == 3 && ud.multimode < 2 && ud.lockout == 0)
    {
        playmusic(&env_music_fn[1][0]);

        flushperms();
        setview(0,0,xdim-1,ydim-1);
        clearview(0L);
        nextpage();

        playanm("vol41a.anm",6);
        clearview(0L);
        nextpage();

        playanm("vol42a.anm",7);
        playanm("vol43a.anm",9);
        clearview(0L);
        nextpage();

        FX_StopAllSounds();
    }

    show_shareware = 26*34;

    ud.level_number =   ln;
    ud.volume_number =  vn;
    ud.player_skill =   sk;
    ud.secretlevel =    0;
    ud.from_bonus = 0;
    parallaxyscale = 0;

    ud.last_level = -1;
    lastsavedpos = -1;
    p->zoom            = 768;
    p->gm              = 0;

    {
        //AddLog("Newgame");
        ResetGameVars();

        InitGameVarPointers();

        ResetSystemDefaults();

        if (ud.m_coop != 1)
        {
            for (i=0;i<MAX_WEAPONS;i++)
            {
                if (aplWeaponWorksLike[i][0]==PISTOL_WEAPON)
                {
                    p->curr_weapon = i;
                    p->gotweapon[i] = 1;
                    p->ammo_amount[i] = 48;
                }
                else if (aplWeaponWorksLike[i][0]==KNEE_WEAPON)
                    p->gotweapon[i] = 1;
                else if (aplWeaponWorksLike[i][0]==HANDREMOTE_WEAPON)
                    p->gotweapon[i] = 1;
            }
            p->last_weapon = -1;
        }
    }
    display_mirror =        0;

    if (ud.multimode > 1)
    {
        if (numplayers < 2)
        {
            connecthead = 0;
            for (i=0;i<MAXPLAYERS;i++) connectpoint2[i] = i+1;
            connectpoint2[ud.multimode-1] = -1;
        }
    }
    else
    {
        connecthead = 0;
        connectpoint2[0] = -1;
    }
}

void resetpspritevars(char g)
{
    short i, j, nexti,circ;
    long firstx,firsty;
    spritetype *s;
    char aimmode[MAXPLAYERS],autoaim[MAXPLAYERS],weaponswitch[MAXPLAYERS];
    STATUSBARTYPE tsbar[MAXPLAYERS];

    EGS(ps[0].cursectnum,ps[0].posx,ps[0].posy,ps[0].posz,
        APLAYER,0,0,0,ps[0].ang,0,0,0,10);

    if (ud.recstat != 2) for (i=0;i<MAXPLAYERS;i++)
        {
            aimmode[i] = ps[i].aim_mode;
            autoaim[i] = ps[i].auto_aim;
            weaponswitch[i] = ps[i].weaponswitch;
            if (ud.multimode > 1 && (gametype_flags[ud.coop]&GAMETYPE_FLAG_PRESERVEINVENTORYDEATH) && ud.last_level >= 0)
            {
                for (j=0;j<MAX_WEAPONS;j++)
                {
                    tsbar[i].ammo_amount[j] = ps[i].ammo_amount[j];
                    tsbar[i].gotweapon[j] = ps[i].gotweapon[j];
                }

                tsbar[i].shield_amount = ps[i].shield_amount;
                tsbar[i].curr_weapon = ps[i].curr_weapon;
                tsbar[i].inven_icon = ps[i].inven_icon;

                tsbar[i].firstaid_amount = ps[i].firstaid_amount;
                tsbar[i].steroids_amount = ps[i].steroids_amount;
                tsbar[i].holoduke_amount = ps[i].holoduke_amount;
                tsbar[i].jetpack_amount = ps[i].jetpack_amount;
                tsbar[i].heat_amount = ps[i].heat_amount;
                tsbar[i].scuba_amount = ps[i].scuba_amount;
                tsbar[i].boot_amount = ps[i].boot_amount;
            }
        }

    resetplayerstats(0);

    for (i=1;i<MAXPLAYERS;i++)
        memcpy(&ps[i],&ps[0],sizeof(ps[0]));

    if (ud.recstat != 2) for (i=0;i<MAXPLAYERS;i++)
        {
            ps[i].aim_mode = aimmode[i];
            ps[i].auto_aim = autoaim[i];
            ps[i].weaponswitch = weaponswitch[i];
            if (ud.multimode > 1 && (gametype_flags[ud.coop]&GAMETYPE_FLAG_PRESERVEINVENTORYDEATH) && ud.last_level >= 0)
            {
                for (j=0;j<MAX_WEAPONS;j++)
                {
                    ps[i].ammo_amount[j] = tsbar[i].ammo_amount[j];
                    ps[i].gotweapon[j] = tsbar[i].gotweapon[j];
                }
                ps[i].shield_amount = tsbar[i].shield_amount;
                ps[i].curr_weapon = tsbar[i].curr_weapon;
                ps[i].inven_icon = tsbar[i].inven_icon;

                ps[i].firstaid_amount = tsbar[i].firstaid_amount;
                ps[i].steroids_amount= tsbar[i].steroids_amount;
                ps[i].holoduke_amount = tsbar[i].holoduke_amount;
                ps[i].jetpack_amount = tsbar[i].jetpack_amount;
                ps[i].heat_amount = tsbar[i].heat_amount;
                ps[i].scuba_amount= tsbar[i].scuba_amount;
                ps[i].boot_amount = tsbar[i].boot_amount;
            }
        }

    numplayersprites = 0;
    circ = 2048/ud.multimode;

    which_palookup = 9;
    j = connecthead;
    i = headspritestat[10];
    while (i >= 0)
    {
        nexti = nextspritestat[i];
        s = &sprite[i];

        if (numplayersprites == MAXPLAYERS)
            gameexit("\nToo many player sprites (max 16.)");

        if (numplayersprites == 0)
        {
            firstx = ps[0].posx;
            firsty = ps[0].posy;
        }

        po[(unsigned char)numplayersprites].ox = s->x;
        po[(unsigned char)numplayersprites].oy = s->y;
        po[(unsigned char)numplayersprites].oz = s->z;
        po[(unsigned char)numplayersprites].oa = s->ang;
        po[(unsigned char)numplayersprites].os = s->sectnum;

        numplayersprites++;
        if (j >= 0)
        {
            s->owner = i;
            s->shade = 0;
            s->xrepeat = 42;
            s->yrepeat = 36;
            s->cstat = 1+256;
            s->xoffset = 0;
            s->clipdist = 64;

            if ((g&MODE_EOL) != MODE_EOL || ps[j].last_extra == 0)
            {
                ps[j].last_extra = max_player_health;
                s->extra = max_player_health;
                ps[j].runspeed = dukefriction;
            }
            else s->extra = ps[j].last_extra;

            s->yvel = j;

            if (!ud.pcolor[j] && ud.multimode > 1 && !(gametype_flags[ud.coop] & GAMETYPE_FLAG_TDM))
            {
                if (s->pal == 0)
                {
                    int k;

                    for (k=0;k<MAXPLAYERS;k++)
                    {
                        if (which_palookup == ps[k].palookup)
                        {
                            which_palookup++;
                            if (which_palookup >= 17)
                                which_palookup = 9;
                            k=0;
                        }
                    }
                    ud.pcolor[j] = s->pal = ps[j].palookup = which_palookup++;
                    if (which_palookup >= 17)
                        which_palookup = 9;
                }
                else ud.pcolor[j] = ps[j].palookup = s->pal;
            }
            else
            {
                int k = ud.pcolor[j];

                if (gametype_flags[ud.coop] & GAMETYPE_FLAG_TDM)
                {
                    switch (ud.pteam[j])
                    {
                    case 0:
                        k = 3;
                        break;
                    case 1:
                        k = 21;
                        break;
                    }
                    ps[j].team = ud.pteam[j];
                }
                s->pal = ps[j].palookup = k;
            }

            ps[j].i = i;
            ps[j].frag_ps = j;
            hittype[i].owner = i;

            hittype[i].bposx = ps[j].bobposx = ps[j].oposx = ps[j].posx =        s->x;
            hittype[i].bposy = ps[j].bobposy = ps[j].oposy = ps[j].posy =        s->y;
            hittype[i].bposz = ps[j].oposz = ps[j].posz =        s->z;
            ps[j].oang  = ps[j].ang  =        s->ang;

            updatesector(s->x,s->y,&ps[j].cursectnum);

            j = connectpoint2[j];

        }
        else deletesprite(i);
        i = nexti;
    }
}

void clearfrags(void)
{
    short i;

    for (i = 0;i<MAXPLAYERS;i++)
        ps[i].frag = ps[i].fraggedself = 0;
    clearbufbyte(&frags[0][0],(MAXPLAYERS*MAXPLAYERS)<<1,0L);
}

void resettimevars(void)
{
    vel = svel = angvel = horiz = 0;

    totalclock = 0L;
    cloudtotalclock = 0L;
    ototalclock = 0L;
    lockclock = 0L;
    ready2send = 1;
}

void genspriteremaps(void)
{
    long j,fp;
    signed char look_pos;
    char *lookfn = "lookup.dat";

    fp = kopen4load(lookfn,0);
    if (fp != -1)
        kread(fp,(char *)&numl,1);
    else
        gameexit("\nERROR: File 'LOOKUP.DAT' not found.");

    for (j=0;j < numl;j++)
    {
        kread(fp,(signed char *)&look_pos,1);
        kread(fp,tempbuf,256);
        makepalookup((long)look_pos,tempbuf,0,0,0,1);
    }

    for (j = 0; j < 256; j++)
        tempbuf[j] = j;
    numl++;
    makepalookup(numl, tempbuf, 15, 15, 15, 1);
    numl++;
    makepalookup(numl, tempbuf, 15, 0, 0, 1);
    numl++;
    makepalookup(numl, tempbuf, 0, 15, 0, 1);
    numl++;
    makepalookup(numl, tempbuf, 0, 0, 15, 1);

    numl -= 3;
    kread(fp,&waterpal[0],768);
    kread(fp,&slimepal[0],768);
    kread(fp,&titlepal[0],768);
    kread(fp,&drealms[0],768);
    kread(fp,&endingpal[0],768);

    palette[765] = palette[766] = palette[767] = 0;
    slimepal[765] = slimepal[766] = slimepal[767] = 0;
    waterpal[765] = waterpal[766] = waterpal[767] = 0;

    kclose(fp);
}

void waitforeverybody()
{
    long i;

    if (numplayers < 2) return;
    packbuf[0] = 250;
    for (i=connecthead;i>=0;i=connectpoint2[i])
    {
        if (i != myconnectindex) sendpacket(i,packbuf,1);
        if ((!networkmode) && (myconnectindex != connecthead)) break; //slaves in M/S mode only send to master
    }
    playerreadyflag[myconnectindex]++;

    while (1)
    {
#ifdef _WIN32
        Sleep(10);
#else
        usleep(10);
#endif
        sampletimer();
        handleevents();
        AudioUpdate();

        if (quitevent || keystatus[1]) gameexit("");

        getpackets();

        for (i=connecthead;i>=0;i=connectpoint2[i])
        {
            if (playerreadyflag[i] < playerreadyflag[myconnectindex]) break;
            if ((!networkmode) && (myconnectindex != connecthead))
            {
                i = -1;
                break;
            } //slaves in M/S mode only wait for master

        }
        if (i < 0) return;
    }
}

void dofrontscreens(char *statustext)
{
    long i=0,j;

    if (ud.recstat != 2)
    {
        if (!statustext)
        {
            //ps[myconnectindex].palette = palette;
            setgamepalette(&ps[myconnectindex], palette, 1);    // JBF 20040308
            fadepal(0,0,0, 0,64,7);
            i = ud.screen_size;
            ud.screen_size = 0;
            vscrn();
            clearview(0L);
        }

        SetGameVarID(g_iReturnVarID,LOADSCREEN, -1, -1);
        OnEvent(EVENT_GETLOADTILE, -1, myconnectindex, -1);
        j = GetGameVarID(g_iReturnVarID, -1, -1);
        rotatesprite(320<<15,200<<15,65536L,0,j > MAXTILES-1?j-MAXTILES:j,0,0,2+8+64,0,0,xdim-1,ydim-1);
        if (j > MAXTILES-1)
        {
            nextpage();
            return;
        }
        if (boardfilename[0] != 0 && ud.level_number == 7 && ud.volume_number == 0)
        {
            menutext(160,90,0,0,"ENTERING USER MAP");
            gametextpal(160,90+10,boardfilename,14,2);
        }
        else
        {
            menutext(160,90,0,0,"ENTERING");
            menutext(160,90+16+8,0,0,level_names[(ud.volume_number*11) + ud.level_number]);
        }

        if (statustext) gametext(160,180,statustext,0,2+8+16);

        nextpage();

        if (!statustext)
        {
            fadepal(0,0,0, 63,0,-7);

            KB_FlushKeyboardQueue();
            ud.screen_size = i;
        }
    }
    else
    {
        if (!statustext)
        {
            clearview(0L);
            //ps[myconnectindex].palette = palette;
            //palto(0,0,0,0);
            setgamepalette(&ps[myconnectindex], palette, 0);    // JBF 20040308
        }
        SetGameVarID(g_iReturnVarID,LOADSCREEN, -1, -1);
        OnEvent(EVENT_GETLOADTILE, -1, myconnectindex, -1);
        j = GetGameVarID(g_iReturnVarID, -1, -1);
        rotatesprite(320<<15,200<<15,65536L,0,j > MAXTILES-1?j-MAXTILES:j,0,0,2+8+64,0,0,xdim-1,ydim-1);
        if (j > MAXTILES-1)
        {
            nextpage();
            return;
        }
        menutext(160,105,0,0,"LOADING...");
        if (statustext) gametext(160,180,statustext,0,2+8+16);
        nextpage();
    }
}

void clearfifo(void)
{
    syncvaltail = 0L;
    syncvaltottail = 0L;
    syncstat = 0;
    bufferjitter = 1;
    mymaxlag = otherminlag = 0;

    movefifoplc = movefifosendplc = fakemovefifoplc = 0;
    avgfvel = avgsvel = avgavel = avghorz = avgbits = avgextbits = 0;
    otherminlag = mymaxlag = 0;

    clearbufbyte(myminlag,MAXPLAYERS<<2,0L);
    clearbufbyte(&loc,sizeof(input),0L);
    clearbufbyte(&sync[0],sizeof(sync),0L);
    clearbufbyte(inputfifo,sizeof(input)*MOVEFIFOSIZ*MAXPLAYERS,0L);

    clearbuf(movefifoend,MAXPLAYERS,0L);
    clearbuf(syncvalhead,MAXPLAYERS,0L);
    clearbuf(myminlag,MAXPLAYERS,0L);

    //    clearbufbyte(playerquitflag,MAXPLAYERS,0x01);
}

void resetmys(void)
{
    myx = omyx = ps[myconnectindex].posx;
    myy = omyy = ps[myconnectindex].posy;
    myz = omyz = ps[myconnectindex].posz;
    myxvel = myyvel = myzvel = 0;
    myang = omyang = ps[myconnectindex].ang;
    myhoriz = omyhoriz = ps[myconnectindex].horiz;
    myhorizoff = omyhorizoff = ps[myconnectindex].horizoff;
    mycursectnum = ps[myconnectindex].cursectnum;
    myjumpingcounter = ps[myconnectindex].jumping_counter;
    myjumpingtoggle = ps[myconnectindex].jumping_toggle;
    myonground = ps[myconnectindex].on_ground;
    myhardlanding = ps[myconnectindex].hard_landing;
    myreturntocenter = ps[myconnectindex].return_to_center;
}

extern void adduserquote(char *daquote);

extern int gotvote[MAXPLAYERS], votes[MAXPLAYERS], voting, vote_map, vote_episode;

int enterlevel(char g)
{
    short i;
    long l;
    char levname[BMAX_PATH];

    if ((g&MODE_DEMO) != MODE_DEMO) ud.recstat = ud.m_recstat;
    ud.respawn_monsters = ud.m_respawn_monsters;
    ud.respawn_items    = ud.m_respawn_items;
    ud.respawn_inventory    = ud.m_respawn_inventory;
    ud.monsters_off = ud.m_monsters_off;
    ud.coop = ud.m_coop;
    ud.marker = ud.m_marker;
    ud.ffire = ud.m_ffire;
    ud.noexits = ud.m_noexits;

    vote_map = vote_episode = voting = -1;
    Bmemset(votes,0,sizeof(votes));
    Bmemset(gotvote,0,sizeof(gotvote));

    if ((g&MODE_DEMO) == 0 && ud.recstat == 2)
        ud.recstat = 0;

    if (VOLUMEALL) Bsprintf(tempbuf,HEAD2);
    else Bsprintf(tempbuf,HEAD);

    if (boardfilename[0] != 0 && ud.m_level_number == 7 && ud.m_volume_number == 0)
    {
        Bstrcpy(levname, boardfilename);
        Bsprintf(apptitle," - %s",levname);
    }
    else Bsprintf(apptitle," - %s",level_names[(ud.volume_number*11)+ud.level_number]);

    Bstrcat(tempbuf,apptitle);
    wm_setapptitle(tempbuf);

    FX_StopAllSounds();
    clearsoundlocks();
    FX_SetReverb(0);

    i = ud.screen_size;
    ud.screen_size = 0;
    dofrontscreens(NULL);
    vscrn();
    ud.screen_size = i;

    if (!VOLUMEONE)
    {

        if (boardfilename[0] != 0 && ud.m_level_number == 7 && ud.m_volume_number == 0)
        {
            if (loadboard(boardfilename,0,&ps[0].posx, &ps[0].posy, &ps[0].posz, &ps[0].ang,&ps[0].cursectnum) == -1)
            {
                initprintf("Map %s not found!\n",boardfilename);
                //gameexit(tempbuf);
                return 1;
            }
            else
            {
                char *p;
                strcpy(levname, boardfilename);
                p = Bstrrchr(levname,'.');
                if (!p) strcat(levname,".mhk");
                else
                {
                    p[1]='m';
                    p[2]='h';
                    p[3]='k';
                    p[4]=0;
                }
                if (!loadmaphack(levname)) initprintf("Loaded map hack file %s\n",levname);
            }
        }
        else if (loadboard(level_file_names[(ud.volume_number*11)+ud.level_number],0,&ps[0].posx, &ps[0].posy, &ps[0].posz, &ps[0].ang,&ps[0].cursectnum) == -1)
        {
            initprintf("Map %s not found!\n",level_file_names[(ud.volume_number*11)+ud.level_number]);
            //gameexit(tempbuf);
            return 1;
        }
        else
        {
            char *p;
            strcpy(levname, level_file_names[(ud.volume_number*11)+ud.level_number]);
            p = Bstrrchr(levname,'.');
            if (!p) strcat(levname,".mhk");
            else
            {
                p[1]='m';
                p[2]='h';
                p[3]='k';
                p[4]=0;
            }
            if (!loadmaphack(levname)) initprintf("Loaded map hack file %s\n",levname);
        }

    }
    else
    {

        l = strlen(level_file_names[(ud.volume_number*11)+ud.level_number]);
        copybufbyte(level_file_names[(ud.volume_number*11)+ud.level_number],&levname[0],l);
        levname[l] = 255;
        levname[l+1] = 0;

        if (loadboard(levname,1,&ps[0].posx, &ps[0].posy, &ps[0].posz, &ps[0].ang,&ps[0].cursectnum) == -1)
        {
            initprintf("Map %s not found!\n",level_file_names[(ud.volume_number*11)+ud.level_number]);
            //gameexit(tempbuf);
            return 1;
        }
        else
        {
            char *p;
            p = Bstrrchr(levname,'.');
            if (!p) strcat(levname,".mhk");
            else
            {
                p[1]='m';
                p[2]='h';
                p[3]='k';
                p[4]=0;
            }
            if (!loadmaphack(levname)) initprintf("Loaded map hack file %s\n",levname);
        }
    }

    precachecount = 0;
    clearbufbyte(gotpic,sizeof(gotpic),0L);
    clearbufbyte(precachehightile, sizeof(precachehightile), 0l);
    //clearbufbyte(hittype,sizeof(hittype),0l); // JBF 20040531: yes? no?

    prelevel(g);

    allignwarpelevators();
    resetpspritevars(g);

    cachedebug = 0;
    automapping = 0;

    if (ud.recstat != 2) MUSIC_StopSong();

    cacheit();

    if (ud.recstat != 2)
    {
        music_select = (ud.volume_number*11) + ud.level_number;
        playmusic(&music_fn[0][(unsigned char)music_select][0]);
    }

    if ((g&MODE_GAME) || (g&MODE_EOL))
        ps[myconnectindex].gm = MODE_GAME;
    else if (g&MODE_RESTART)
    {
        if (ud.recstat == 2)
            ps[myconnectindex].gm = MODE_DEMO;
        else ps[myconnectindex].gm = MODE_GAME;
    }

    if ((ud.recstat == 1) && (g&MODE_RESTART) != MODE_RESTART)
        opendemowrite();

    if (VOLUMEONE)
    {
        if (ud.level_number == 0 && ud.recstat != 2) FTA(40,&ps[myconnectindex]);
    }

    for (i=connecthead;i>=0;i=connectpoint2[i])
        switch (dynamictostatic[sector[sprite[ps[i].i].sectnum].floorpicnum])
        {
        case HURTRAIL__STATIC:
        case FLOORSLIME__STATIC:
        case FLOORPLASMA__STATIC:
            resetweapons(i);
            resetinventory(i);
            ps[i].gotweapon[PISTOL_WEAPON] = 0;
            ps[i].ammo_amount[PISTOL_WEAPON] = 0;
            ps[i].curr_weapon = KNEE_WEAPON;
            ps[i].kickback_pic = 0;
            break;
        }

    //PREMAP.C - replace near the my's at the end of the file

    resetmys();

    //ps[myconnectindex].palette = palette;
    //palto(0,0,0,0);
    setgamepalette(&ps[myconnectindex], palette, 0);    // JBF 20040308

    setpal(&ps[myconnectindex]);
    flushperms();

    everyothertime = 0;
    global_random = 0;

    ud.last_level = ud.level_number+1;

    clearfifo();

    for (i=numinterpolations-1;i>=0;i--) bakipos[i] = *curipos[i];

    restorepalette = 1;

    flushpackets();
    waitforeverybody();

    palto(0,0,0,0);
    vscrn();
    clearview(0L);
    drawbackground();
    displayrooms(myconnectindex,65536);

    clearbufbyte(playerquitflag,MAXPLAYERS,0x01010101);
    ps[myconnectindex].over_shoulder_on = 0;

    clearfrags();

    resettimevars();  // Here we go

    //Bsprintf(g_szBuf,"ENTERLEVEL L=%d V=%d",ud.level_number, ud.volume_number);
    //AddLog(g_szBuf);
    // variables are set by pointer...

    OnEvent(EVENT_ENTERLEVEL, -1, -1, -1);
    return 0;
}
