//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "serializer.h"
#include "mapinfo.h"
#include "duke3d.h"


BEGIN_DUKE_NS

FSerializer& Serialize(FSerializer& arc, const char* keyname, animwalltype& w, animwalltype* def)
{
    if (arc.BeginObject(keyname))
    {
      arc("wallnum", w.wallnum)
        ("tag", w.tag)
        .EndObject();
    }
    return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, player_orig& w, player_orig* def)
{
    if (arc.BeginObject(keyname))
    {
      arc("ox", w.ox)
        ("oy", w.oy)
        ("oz", w.oz)
        ("oa", w.oa)
        ("os", w.os)
        .EndObject();
    }
    return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, player_struct& w, player_struct* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("posx", w.posx)
            ("posy", w.posy)
            ("posz", w.posz)
            ("q16ang", w.q16ang)
            ("q16horiz", w.q16horiz)
            ("q16horizoff", w.q16horizoff)
            ("q16rotscrnang", w.q16rotscrnang)
            ("q16look_ang", w.q16look_ang)
            ("one_eighty_count", w.one_eighty_count)
            ("gotweapon", w.gotweapon)
            ("palette", w.palette)
            ("pals", w.pals)
            ("fricx", w.fric.x)
            ("fricy", w.fric.y)
            ("zoom", w.zoom)
            ("exitx", w.exitx)
            ("exity", w.exity)
            ("numloogs", w.numloogs)
            ("loogcnt", w.loogcnt)
            .Array("loogiex", w.loogiex, w.numloogs)
            .Array("loogiey", w.loogiey, w.numloogs)
            ("bobposx", w.bobposx)
            ("bobposy", w.bobposy)
            ("pyoff", w.pyoff)
            ("posxv", w.posxv)
            ("posyv", w.posyv)
            ("poszv", w.poszv)
            ("last_pissed_time", w.last_pissed_time)
            ("truefz", w.truefz)
            ("truecz", w.truecz)
            ("player_par", w.player_par)
            ("visibility", w.visibility)
            ("bobcounter", w.bobcounter)
            ("weapon_sway", w.weapon_sway)
            ("pals_time", w.pals_time)
            ("randomflamex", w.randomflamex)
            ("crack_time", w.crack_time)
            ("aim.mode", w.aim_mode)
            ("auto_aim", w.auto_aim)
            ("weaponswitch", w.weaponswitch)
            ("angvel", w.angvel)
            ("cursectnum", w.cursectnum)
            ("last_extra", w.last_extra)
            ("subweapon", w.subweapon)
            .Array("ammo_count", w.ammo_amount, MAX_WEAPONS)
            ("wackedbyactor", w.wackedbyactor)
            ("frag", w.frag)
            ("fraggedself", w.fraggedself)
            ("curr_weapon", w.curr_weapon)
            ("last_weapon", w.last_weapon)
            ("tipincs", w.tipincs)
            ("wantweaponfire", w.wantweaponfire)
            ("holoduke_amount", w.holoduke_amount)
            ("newowner", w.newowner)
            ("hurt_delay", w.hurt_delay)
            ("hbomb_hold_delay", w.hbomb_hold_delay)
            ("jumping_counter", w.jumping_counter)
            ("airleft", w.airleft)
            ("knee_incs", w.knee_incs)
            ("access_incs", w.access_incs)
            ("ftq", w.ftq)
            ("access_wallnum", w.access_wallnum)
            ("access_spritenum", w.access_spritenum)
            ("kickback_pic", w.kickback_pic)
            ("got_access", w.got_access)
            ("weapon_ang", w.weapon_ang)
            ("firstaid_amount", w.firstaid_amount)
            ("somethingonplayer", w.somethingonplayer)
            ("on_crane", w.on_crane)
            ("i", w.i)
            ("one_parallax_sectnum", w.one_parallax_sectnum)
            ("over_shoulder_on", w.over_shoulder_on)
            ("random_club_frame", w.random_club_frame)
            ("fist_incs", w.fist_incs)
            ("dummyplayersprite", w.dummyplayersprite)
            ("extra_extra8", w.extra_extra8)
            ("quick_kick", w.quick_kick)
            ("heat_amount", w.heat_amount)
            ("actorsqu", w.actorsqu)
            ("timebeforeexit", w.timebeforeexit)
            ("customexitsound", w.customexitsound)
            ("weapreccnt", w.weapreccnt)
            .Array("weaprecs", w.weaprecs, w.weapreccnt)
            ("interface_toggle_flag", w.interface_toggle_flag)
            ("dead_flag", w.dead_flag)
            ("show_empty_weapon", w.show_empty_weapon)
            ("scuba_amount", w.scuba_amount)
            ("jetpack_amount", w.jetpack_amount)
            ("steroids_amount", w.steroids_amount)
            ("shield_amount", w.shield_amount)
            ("holoduke_on", w.holoduke_on)
            ("pycount", w.pycount)
            ("weapon_pos", w.weapon_pos)
            ("frag_ps", w.frag_ps)
            ("transporter_hold", w.transporter_hold)
            ("last_full_weapon", w.last_full_weapon)
            ("footprintshade", w.footprintshade)
            ("boot_amount", w.boot_amount)
            ("scream_voice", w.scream_voice)
            ("gm", w.gm)
            ("on_warping_sector", w.on_warping_sector)
            ("footprintcount", w.footprintcount)
            ("hbomb_on", w.hbomb_on)
            ("jumping_toggle", w.jumping_toggle)
            ("rapid_fire_hold", w.rapid_fire_hold)
            ("on_ground", w.on_ground)
            .Array("name", w.name, 32)
            ("inven_icon", w.inven_icon)
            ("buttonpalette", w.buttonpalette)
            ("jetpack_on", w.jetpack_on)
            ("spritebridge", w.spritebridge)
            ("lastrandomspot", w.lastrandomspot)
            ("scuba_on", w.scuba_on)
            ("footprintpal", w.footprintpal)
            ("heat_on", w.heat_on)
            ("holster_weapon", w.holster_weapon)
            ("falling_counter", w.falling_counter)
            ("refresh_inventory", w.refresh_inventory)
            ("toggle_key_flag", w.toggle_key_flag)
            ("knuckle_incs", w.knuckle_incs)
            ("walking_snd_toggle", w.walking_snd_toggle)
            ("palookup", w.palookup)
            ("hard_landing", w.hard_landing)
            ("return_to_center", w.return_to_center)
            ("max_secret_rooms", w.max_secret_rooms)
            ("secret_rooms", w.secret_rooms)
            ("max_actors_killed", w.max_actors_killed)
            ("actors_killed", w.actors_killed)
            // RR from here on
            ("stairs", w.stairs)
            ("detonate_count", w.detonate_count)
            ("noise_x", w.noise_x)
            ("noise_y", w.noise_y)
            ("noise_radius", w.noise_radius)
            ("drink_timer", w.drink_timer)
            ("eat_timer", w.eat_timer)
            ("slotwin", w.SlotWin)
            ("recoil", w.recoil)
            ("detonate_time", w.detonate_time)
            ("yehaa_timer", w.yehaa_timer)
            ("drink_amt", w.drink_amt)
            ("eat", w.eat)
            ("drunkang", w.drunkang)
            ("eatang", w.eatang)
            .Array("shotgun_state", w.shotgun_state, 2)
            ("donoise", w.donoise)
            .Array("keys", w.keys, 5)
            // RRRA from here on
            ("drug_aspect", w.drug_aspect)
            ("drug_timer", w.drug_timer)
            ("seasick", w.SeaSick)
            ("mamaend", w.MamaEnd)
            ("motospeed", w.MotoSpeed)
            ("moto_drink", w.moto_drink)
            ("tiltstatus", w.TiltStatus)
            ("vbumpnow", w.VBumpNow)
            ("vbumptarget", w.VBumpTarget)
            ("turbcount", w.TurbCount)
            .Array("drug_stat", w.drug_stat, 3)
            ("drugmode", w.DrugMode)
            ("lotag800kill", w.lotag800kill)
            ("sea_sick_stat", w.sea_sick_stat)
            ("hurt_delay2", w.hurt_delay2)
            ("nocheat", w.nocheat)
            ("onmotorcycle", w.OnMotorcycle)
            ("onboat", w.OnBoat)
            ("moto_underwater", w.moto_underwater)
            ("notonwater", w.NotOnWater)
            ("motoonground", w.MotoOnGround)
            ("moto_do_bump", w.moto_do_bump)
            ("moto_bump_fast", w.moto_bump_fast)
            ("moto_on_oil", w.moto_on_oil)
            ("moto_on_mud", w.moto_on_mud)
            ("crouch_toggle", w.crouch_toggle)
            .EndObject();

        w.invdisptime = 0;
        w.oq16ang = w.q16ang;
        w.oq16horiz = w.q16horiz;
        w.oq16horizoff = w.q16horizoff;
        w.oq16rotscrnang = w.q16rotscrnang;
        w.oposx = w.posx;
        w.oposy = w.posy;
        w.oposz = w.posz;
        w.opyoff = w.pyoff;
        w.horizAngleAdjust = 0;
        w.horizSkew = 0;
        w.lookLeft = false;
        w.lookRight = false;
    }
    return arc;
}

FSerializer& Serialize(FSerializer& arc, const char* keyname, weaponhit& w, weaponhit* def)
{
    if (arc.BeginObject(keyname))
    {
        arc("cgg", w.cgg)
            ("picnum", w.picnum)
            ("ang", w.ang)
            ("extra", w.extra)
            ("owner", w.owner)
            ("movflag", w.movflag)
            ("tempang", w.tempang)
            ("actorstayput", w.actorstayput)
            ("dispicnum", w.dispicnum)
            ("timetosleep", w.timetosleep)
            ("floorz", w.floorz)
            ("ceilingz", w.ceilingz)
            ("lastvx", w.lastvx)
            ("lastvy", w.lastvy)
            ("bposx", w.bposx)
            ("bposy", w.bposy)
            ("bposz", w.bposz)
            ("aflags", w.aflags)
            .Array("temp_data", w.temp_data, 6)
            .EndObject();
    }
    return arc;
}


void SerializeGlobals(FSerializer& arc)
{
    if (arc.isReading())
    {
        memset(sectorextra, 0, sizeof(sectorextra));
        memset(spriteextra, 0, sizeof(spriteextra));
        memset(shadedsector, 0, sizeof(shadedsector));
        memset(geosectorwarp, -1, sizeof(geosectorwarp));
        memset(geosectorwarp2, -1, sizeof(geosectorwarp2));
        memset(ambienthitag, -1, sizeof(ambienthitag));
        memset(ambientlotag, -1, sizeof(ambientlotag));
    }
    if (arc.BeginObject("globals"))
    {
        arc("multimode", ud.multimode);
        if (ud.multimode > 1) arc.Array("frags", &frags[0][0], MAXPLAYERS * MAXPLAYERS);
        arc("skill", ud.player_skill)

            ("from_bonus", ud.from_bonus)
            ("secretlevel", ud.secretlevel)
            ("respawn_monsters", ud.respawn_monsters)
            ("respawn_items", ud.respawn_items)
            ("respawn_inventory", ud.respawn_inventory)
            ("god", ud.god)
            //("auto_run", ud.auto_run)
            ("monsters_off", ud.monsters_off)
            ("last_level", ud.last_level)
            ("eog", ud.eog)
            ("coop", ud.coop)
            ("marker", ud.marker)
            ("ffire", ud.ffire)

            .Array("spriteextra", spriteextra, MAXSPRITES)
            .Array("weaponhit", hittype, MAXSPRITES)
            .Array("sectorextra", sectorextra, numsectors)
            ("rtsplaying", rtsplaying)
            ("tempwallptr", tempwallptr)
            ("sound445done", sound445done)
            ("leveltexttime", levelTextTime)
            .Array("players", ps, ud.multimode)
            ("spriteqamount", spriteqamount)
            .Array("shadedsector", shadedsector, numsectors)
            ("lastvisinc", lastvisinc)
            ("numanimwalls", numanimwalls)
            .Array("animwall", animwall, numanimwalls)
            ("camsprite", camsprite)
            ("earthquaketime", earthquaketime)
            ("freezerhurtowner", freezerhurtowner)
            ("global_random", global_random)
            ("impact_damage", impact_damage)
            ("numplayersprites", numplayersprites)
            ("spriteqloc", spriteqloc)
            ("animatecnt", animatecnt)

            .Array("animatesect", animatesect, animatecnt)
            .Array("animatetype", animatetype, animatecnt)
           .Array("animatetarget", animatetarget, animatecnt)
            .Array("animategoal", animategoal, animatecnt)
            .Array("animatevel", animatevel, animatecnt)

            ("numclouds", numclouds)
            ("cloudx", cloudx)
            ("cloudy", cloudy)
            ("cloudtotalclock", cloudtotalclock)
            .Array("clouds", clouds, numclouds)

            .Array("spriteq", spriteq, 1024)
            ("numcyclers", numcyclers)
            .Array("cyclers", &cyclers[0][0], 6 * numcyclers)
            ("mirrorcnt", mirrorcnt)
            .Array("mirrorsector", mirrorsector, mirrorcnt)
            .Array("mirrorwall", mirrorwall, mirrorcnt)
            ("lockclock", lockclock)
            ("wupass", wupass)
            ("chickenplant", chickenplant)
            ("thunderon", thunderon)
            ("ufospawn", ufospawn)
            ("ufocnt", ufocnt)
            ("hulkspawn", hulkspawn)
            ("lastlevel", lastlevel)
            ("geocnt", geocnt)
            .Array("geosectorwarp", geosectorwarp, geocnt)
            .Array("geosectorwarp2", geosectorwarp2, geocnt)
            .Array("geosector", geosector, geocnt)
            .Array("geox", geox, geocnt)
            .Array("geoy", geoy, geocnt)
            .Array("geox2", geox2, geocnt)
            .Array("geoy2", geoy2, geocnt)
            ("ambientfx", ambientfx)
            .Array("ambientlotag", ambientlotag, ambientfx)
            .Array("ambienthitag", ambienthitag, ambientfx)
            .Array("msx", msx, MAXANIMPOINTS)
            .Array("msy", msy, MAXANIMPOINTS)
            ("windtime", WindTime)
            ("winddir", WindDir)
            ("fakebubba_spawn", fakebubba_spawn)
            ("mamaspawn_count", mamaspawn_count)
            ("banjosound", banjosound)
            ("belltime", BellTime)
            ("bellsprite", BellSprite)
            ("enemysizecheat", enemysizecheat)
            ("ufospawnsminion", ufospawnsminion)
            ("pistonsound", pistonsound)
            ("chickenphase", chickenphase)
            ("RRRA_ExitedLevel", RRRA_ExitedLevel)
            ("fogactive", fogactive)
            ("everyothertime", everyothertime)
            .Array("po", po, ud.multimode)
            .EndObject();

        ud.m_player_skill = ud.player_skill;
        ud.m_respawn_monsters = ud.respawn_monsters;
        ud.m_respawn_items = ud.respawn_items;
        ud.m_respawn_inventory = ud.respawn_inventory;
        ud.m_monsters_off = ud.monsters_off;
        ud.m_coop = ud.coop;
        ud.m_marker = ud.marker;
        ud.m_ffire = ud.ffire;


    }
}

#if 0
    if (arc.isReading())
    {
     if(ps[myconnectindex].over_shoulder_on != 0)
     {
         cameradist = 0;
         cameraclock = 0;
         ps[myconnectindex].over_shoulder_on = 1;
     }

     screenpeek = myconnectindex;

     clearbufbyte(gotpic,sizeof(gotpic),0L);
     clearsoundlocks();
         cacheit();

     music_select = (ud.volume_number*11) + ud.level_number;
     playmusic(&music_fn[0][music_select][0]);

     ps[myconnectindex].gm = MODE_GAME;
         ud.recstat = 0;

     if(ps[myconnectindex].jetpack_on)
         spritesound(DUKE_JETPACK_IDLE,ps[myconnectindex].i);

     restorepalette = 1;
     setpal(&ps[myconnectindex]);
     vscrn();

     FX_SetReverb(0);


     numinterpolations = 0;
     startofdynamicinterpolations = 0;

     k = headspritestat[3];
     while(k >= 0)
     {
        switch(sprite[k].lotag)
        {
            case 31:
                setinterpolation(&sector[sprite[k].sectnum].floorz);
                break;
            case 32:
                setinterpolation(&sector[sprite[k].sectnum].ceilingz);
                break;
            case 25:
                setinterpolation(&sector[sprite[k].sectnum].floorz);
                setinterpolation(&sector[sprite[k].sectnum].ceilingz);
                break;
            case 17:
                setinterpolation(&sector[sprite[k].sectnum].floorz);
                setinterpolation(&sector[sprite[k].sectnum].ceilingz);
                break;
            case 0:
            case 5:
            case 6:
            case 11:
            case 14:
            case 15:
            case 16:
            case 26:
            case 30:
                setsectinterpolate(k);
                break;
        }

        k = nextspritestat[k];
     }

     for(i=numinterpolations-1;i>=0;i--) bakipos[i] = *curipos[i];
     for(i = animatecnt-1;i>=0;i--)
         setinterpolation(animateptr(i));

     show_shareware = 0;
     everyothertime = 0;

     clearbufbyte(playerquitflag,MAXPLAYERS,0x01010101);

     resetmys();

     ready2send = 1;

     flushpackets();
     clearfifo();
     waitforeverybody();

     resettimevars();

     return(0);
corrupt:
     Bsprintf(buf,"Save game file \"%s\" is corrupt.",fnptr);
     gameexit(buf);
     return -1;
}

int saveplayer(signed char spot)
{
    int i, j;
    char fn[13];
    char mpfn[13];
    char *fnptr;
    FILE *fil;
    int bv = BYTEVERSION;
    int ptrbuf[MAXTILES];

    assert(MAXTILES > MAXANIMATES);

    strcpy(fn, "game0.sav");
    strcpy(mpfn, "gameA_00.sav");

    if(spot < 0)
    {
        multiflag = 1;
        multiwhat = 1;
        multipos = -spot-1;
        return -1;
    }

    waitforeverybody();

    if( multiflag == 2 && multiwho != myconnectindex )
    {
        fnptr = mpfn;
        mpfn[4] = spot + 'A';

        if(ud.multimode > 9)
        {
            mpfn[6] = (multiwho/10) + '0';
            mpfn[7] = multiwho + '0';
        }
        else mpfn[7] = multiwho + '0';
    }
    else
    {
        fnptr = fn;
        fn[4] = spot + '0';
    }

    if ((fil = fopen(fnptr,"wb")) == 0) return(-1);

    ready2send = 0;

    dfwrite(&bv,4,1,fil);
    dfwrite(&ud.multimode,sizeof(ud.multimode),1,fil);

    dfwrite(&ud.savegame[spot][0],19,1,fil);
    dfwrite(&ud.volume_number,sizeof(ud.volume_number),1,fil);
    dfwrite(&ud.level_number,sizeof(ud.level_number),1,fil);
    dfwrite(&ud.player_skill,sizeof(ud.player_skill),1,fil);
    dfwrite(&boardfilename[0],BMAX_PATH,1,fil);

    if (!waloff[TILE_SAVESHOT]) {
        walock[TILE_SAVESHOT] = 254;
        allocache((void **)&waloff[TILE_SAVESHOT],200*320,&walock[TILE_SAVESHOT]);
        clearbuf((void*)waloff[TILE_SAVESHOT],(200*320)/4,0);
        walock[TILE_SAVESHOT] = 1;
    }
    dfwrite((char *)waloff[TILE_SAVESHOT],320,200,fil);

    dfwrite(&numwalls,2,1,fil);
    dfwrite(&wall[0],sizeof(walltype),MAXWALLS,fil);
    dfwrite(&numsectors,2,1,fil);
    dfwrite(&sector[0],sizeof(sectortype),MAXSECTORS,fil);
    dfwrite(&sprite[0],sizeof(spritetype),MAXSPRITES,fil);
    dfwrite(&spriteext[0],sizeof(spriteexttype),MAXSPRITES,fil);
    dfwrite(&headspritesect[0],2,MAXSECTORS+1,fil);
    dfwrite(&prevspritesect[0],2,MAXSPRITES,fil);
    dfwrite(&nextspritesect[0],2,MAXSPRITES,fil);
    dfwrite(&headspritestat[0],2,MAXSTATUS+1,fil);
    dfwrite(&prevspritestat[0],2,MAXSPRITES,fil);
    dfwrite(&nextspritestat[0],2,MAXSPRITES,fil);
    dfwrite(&numcyclers,sizeof(numcyclers),1,fil);
    dfwrite(&cyclers[0][0],12,MAXCYCLERS,fil);
    dfwrite(ps,sizeof(ps),1,fil);
    dfwrite(po,sizeof(po),1,fil);
    dfwrite(&numanimwalls,sizeof(numanimwalls),1,fil);
    dfwrite(&animwall,sizeof(animwall),1,fil);
    dfwrite(&msx[0],sizeof(int),sizeof(msx)/sizeof(int),fil);
    dfwrite(&msy[0],sizeof(int),sizeof(msy)/sizeof(int),fil);
    dfwrite(&spriteqloc,sizeof(short),1,fil);
    dfwrite(&spriteqamount,sizeof(short),1,fil);
    dfwrite(&spriteq[0],sizeof(short),spriteqamount,fil);
    dfwrite(&mirrorcnt,sizeof(short),1,fil);
    dfwrite(&mirrorwall[0],sizeof(short),64,fil);
    dfwrite(&mirrorsector[0],sizeof(short),64,fil);
    dfwrite(&show2dsector[0],sizeof(char),MAXSECTORS>>3,fil);
    dfwrite(&actortype[0],sizeof(char),MAXTILES,fil);

    dfwrite(&numclouds,sizeof(numclouds),1,fil);
    dfwrite(&clouds[0],sizeof(short)<<7,1,fil);
    dfwrite(&cloudx[0],sizeof(short)<<7,1,fil);
    dfwrite(&cloudy[0],sizeof(short)<<7,1,fil);

    dfwrite(&script[0],4,MAXSCRIPTSIZE,fil);

    memset(ptrbuf, 0, sizeof(ptrbuf));
    for(i=0;i<MAXTILES;i++)
        if(actorscrptr[i])
        {
            ptrbuf[i] = (int)((intptr_t)actorscrptr[i] - (intptr_t)&script[0]);
        }
    dfwrite(&ptrbuf[0],4,MAXTILES,fil);

    dfwrite(&lockclock,sizeof(lockclock),1,fil);
    dfwrite(&pskybits,sizeof(pskybits),1,fil);
    dfwrite(&pskyoff[0],sizeof(pskyoff[0]),MAXPSKYTILES,fil);
    dfwrite(&animatecnt,sizeof(animatecnt),1,fil);
    dfwrite(&animatesect[0],2,MAXANIMATES,fil);
    dfwrite(&ptrbuf[0],4,MAXANIMATES,fil);
    dfwrite(&animategoal[0],4,MAXANIMATES,fil);
    dfwrite(&animatevel[0],4,MAXANIMATES,fil);

    dfwrite(&earthquaketime,sizeof(earthquaketime),1,fil);
    dfwrite(&ud.from_bonus,sizeof(ud.from_bonus),1,fil);
    dfwrite(&ud.secretlevel,sizeof(ud.secretlevel),1,fil);
    dfwrite(&ud.respawn_monsters,sizeof(ud.respawn_monsters),1,fil);
    dfwrite(&ud.respawn_items,sizeof(ud.respawn_items),1,fil);
    dfwrite(&ud.respawn_inventory,sizeof(ud.respawn_inventory),1,fil);
    dfwrite(&ud.god,sizeof(ud.god),1,fil);
    dfwrite(&ud.auto_run,sizeof(ud.auto_run),1,fil);
    dfwrite(&ud.crosshair,sizeof(ud.crosshair),1,fil);
    dfwrite(&ud.monsters_off,sizeof(ud.monsters_off),1,fil);
    dfwrite(&ud.last_level,sizeof(ud.last_level),1,fil);
    dfwrite(&ud.eog,sizeof(ud.eog),1,fil);
    dfwrite(&ud.coop,sizeof(ud.coop),1,fil);
    dfwrite(&ud.marker,sizeof(ud.marker),1,fil);
    dfwrite(&ud.ffire,sizeof(ud.ffire),1,fil);
    dfwrite(&camsprite,sizeof(camsprite),1,fil);
    dfwrite(&connecthead,sizeof(connecthead),1,fil);
    dfwrite(connectpoint2,sizeof(connectpoint2),1,fil);
    dfwrite(&numplayersprites,sizeof(numplayersprites),1,fil);
    dfwrite((short *)&frags[0][0],sizeof(frags),1,fil);

    dfwrite(&randomseed,sizeof(randomseed),1,fil);
    dfwrite(&global_random,sizeof(global_random),1,fil);
    dfwrite(&parallaxyscale,sizeof(parallaxyscale),1,fil);

    fclose(fil);

    if(ud.multimode < 2)
    {
    strcpy(fta_quotes[122],"GAME SAVED");
    FTA(122,&ps[myconnectindex]);
    }

    ready2send = 1;

    waitforeverybody();

    ototalclock = totalclock;

    return(0);
}
#endif
END_DUKE_NS
