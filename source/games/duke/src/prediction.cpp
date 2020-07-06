//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2020 - Christoph Oelckers

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
aint with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
//------------------------------------------------------------------------- 

#include "ns.h"	// Must come before everything else!

#include "duke3d.h"
#include "sbar.h"
#include "screens.h"
#include "baselayer.h"
#include "m_argv.h"
#include "mapinfo.h"
#include "texturemanager.h"

BEGIN_DUKE_NS 


int myx, omyx, myxvel, myy, omyy, myyvel, myz, omyz, myzvel;
short myhoriz, omyhoriz, myhorizoff, omyhorizoff, globalskillsound;
short myang, omyang, mycursectnum, myjumpingcounter;
char myjumpingtoggle, myonground, myhardlanding,myreturntocenter;
int fakemovefifoplc;
int myxbak[MOVEFIFOSIZ], myybak[MOVEFIFOSIZ], myzbak[MOVEFIFOSIZ];
int myhorizbak[MOVEFIFOSIZ];
short myangbak[MOVEFIFOSIZ];


void resetmys()
{
      myx = omyx = ps[myconnectindex].posx;
	  myy = omyy = ps[myconnectindex].posy;
	  myz = omyz = ps[myconnectindex].posz;
	  myxvel = myyvel = myzvel = 0;
	  myang = omyang = ps[myconnectindex].getang();
	  myhoriz = omyhoriz = ps[myconnectindex].gethoriz();
	  myhorizoff = omyhorizoff = ps[myconnectindex].gethorizof();
	  mycursectnum = ps[myconnectindex].cursectnum;
	  myjumpingcounter = ps[myconnectindex].jumping_counter;
	  myjumpingtoggle = ps[myconnectindex].jumping_toggle;
      myonground = ps[myconnectindex].on_ground;
      myhardlanding = ps[myconnectindex].hard_landing;
      myreturntocenter = ps[myconnectindex].return_to_center;
 }

#if 0 // todo: fix this when networking works again
void fakedomovethingscorrect(void)
{
     int i;
     struct player_struct *p;

     if (numplayers < 2) return;

     i = ((movefifoplc-1)&(MOVEFIFOSIZ-1));
     p = &ps[myconnectindex];

     if (p->posx == myxbak[i] && p->posy == myybak[i] && p->posz == myzbak[i]
          && p->horiz == myhorizbak[i] && p->ang == myangbak[i]) return;

     myx = p->posx; omyx = p->oposx; myxvel = p->posxv;
     myy = p->posy; omyy = p->oposy; myyvel = p->posyv;
     myz = p->posz; omyz = p->oposz; myzvel = p->poszv;
     myang = p->ang; omyang = p->oang;
     mycursectnum = p->cursectnum;
     myhoriz = p->horiz; omyhoriz = p->ohoriz;
     myhorizoff = p->horizoff; omyhorizoff = p->ohorizoff;
     myjumpingcounter = p->jumping_counter;
     myjumpingtoggle = p->jumping_toggle;
     myonground = p->on_ground;
     myhardlanding = p->hard_landing;
     myreturntocenter = p->return_to_center;

     fakemovefifoplc = movefifoplc;
     while (fakemovefifoplc < movefifoend[myconnectindex])
          fakedomovethings();

}

void fakedomovethings(void)
{
        input *syn;
        struct player_struct *p;
        int i, j, k, doubvel, fz, cz, hz, lz, x, y;
        unsigned int sb_snum;
        short psect, psectlotag, tempsect, backcstat;
        uint8_t shrunk, spritebridge;

        syn = (input *)&inputfifo[fakemovefifoplc&(MOVEFIFOSIZ-1)][myconnectindex];

        p = &ps[myconnectindex];

        backcstat = sprite[p->i].cstat;
        sprite[p->i].cstat &= ~257;

        sb_snum = syn->bits;

        psect = mycursectnum;
        psectlotag = sector[psect].lotag;
        spritebridge = 0;

        shrunk = (sprite[p->i].yrepeat < (isRR()? 8 : 32));

        if( ud.clipping == 0 && ( sector[psect].floorpicnum == MIRROR || psect < 0 || psect >= MAXSECTORS) )
        {
            myx = omyx;
            myy = omyy;
        }
        else
        {
            omyx = myx;
            omyy = myy;
        }

        omyhoriz = myhoriz;
        omyhorizoff = myhorizoff;
        omyz = myz;
        omyang = myang;

        getzrange(myx,myy,myz,psect,&cz,&hz,&fz,&lz,163L,CLIPMASK0);

        j = getflorzofslope(psect,myx,myy);

        if( (lz&49152) == 16384 && psectlotag == 1 && klabs(myz-j) > PHEIGHT+(16<<8) )
            psectlotag = 0;

        if( p->aim_mode == 0 && myonground && psectlotag != 2 && (sector[psect].floorstat&2) )
        {
                x = myx+(sintable[(myang+512)&2047]>>5);
                y = myy+(sintable[myang&2047]>>5);
                tempsect = psect;
                updatesector(x,y,&tempsect);
                if (tempsect >= 0)
                {
                     k = getflorzofslope(psect,x,y);
                     if (psect == tempsect)
                          myhorizoff += mulscale16(j-k,160);
                     else if (klabs(getflorzofslope(tempsect,x,y)-k) <= (4<<8))
                          myhorizoff += mulscale16(j-k,160);
                }
        }
        if (myhorizoff > 0) myhorizoff -= ((myhorizoff>>3)+1);
        else if (myhorizoff < 0) myhorizoff += (((-myhorizoff)>>3)+1);

        if(hz >= 0 && (hz&49152) == 49152)
        {
                hz &= (MAXSPRITES-1);
                if (sprite[hz].statnum == 1 && sprite[hz].extra >= 0)
                {
                    hz = 0;
                    cz = getceilzofslope(psect,myx,myy);
                }
        }

        if(lz >= 0 && (lz&49152) == 49152)
        {
                 j = lz&(MAXSPRITES-1);
                 if ((sprite[j].cstat&33) == 33)
                 {
                        psectlotag = 0;
                        spritebridge = 1;
                 }
                 if(badguy(&sprite[j]) && sprite[j].xrepeat > 24 && klabs(sprite[p->i].z-sprite[j].z) < (84<<8) )
                 {
                    j = getangle( sprite[j].x-myx,sprite[j].y-myy);
                    myxvel -= sintable[(j+512)&2047]<<4;
                    myyvel -= sintable[j&2047]<<4;
                }
        }

        if( sprite[p->i].extra <= 0 )
        {
                 if( psectlotag == 2 )
                 {
                            if(p->on_warping_sector == 0)
                            {
                                     if( klabs(myz-fz) > (PHEIGHT>>1))
                                             myz += 348;
                            }
                            clipmove(&myx,&myy,&myz,&mycursectnum,0,0,164L,(4L<<8),(4L<<8),CLIPMASK0);
                 }

                 updatesector(myx,myy,&mycursectnum);
                 pushmove(&myx,&myy,&myz,&mycursectnum,128L,(4L<<8),(20L<<8),CLIPMASK0);

                myhoriz = 100;
                myhorizoff = 0;

                 goto ENDFAKEPROCESSINPUT;
        }

        doubvel = TICSPERFRAME;

        if(p->on_crane >= 0) goto FAKEHORIZONLY;

        if(p->one_eighty_count < 0) myang += 128;

        i = 40;

        if( psectlotag == 2)
        {
                 myjumpingcounter = 0;

                 if ( (sb_snum&1) && !(p->OnMotorcycle || p->OnBoat) )
                 {
                            if(myzvel > 0) myzvel = 0;
                            myzvel -= 348;
                            if(myzvel < -(256*6)) myzvel = -(256*6);
                 }
                 else if ( (sb_snum&(1<<1)) && !(p->OnMotorcycle || p->OnBoat) )
                 {
                            if(myzvel < 0) myzvel = 0;
                            myzvel += 348;
                            if(myzvel > (256*6)) myzvel = (256*6);
                 }
                 else
                 {
                    if(myzvel < 0)
                    {
                        myzvel += 256;
                        if(myzvel > 0)
                            myzvel = 0;
                    }
                    if(myzvel > 0)
                    {
                        myzvel -= 256;
                        if(myzvel < 0)
                            myzvel = 0;
                    }
                }

                if(myzvel > 2048) myzvel >>= 1;

                 myz += myzvel;

                 if(myz > (fz-(15<<8)) )
                            myz += ((fz-(15<<8))-myz)>>1;

                 if(myz < (cz+(4<<8)) )
                 {
                            myz = cz+(4<<8);
                            myzvel = 0;
                 }
        }

        else if(p->jetpack_on)
        {
                 myonground = 0;
                 myjumpingcounter = 0;
                 myhardlanding = 0;

                 if(p->jetpack_on < 11)
                            myz -= (p->jetpack_on<<7); //Goin up

                 if(shrunk) j = 512;
                 else j = 2048;
                 
                 if ((sb_snum&1) && !(p->OnMotorcycle || p->OnBoat))
                            myz -= j;
                 if ((sb_snum&(1<<1)) && !(p->OnMotorcycle || p->OnBoat))
                            myz += j;

                 if(shrunk == 0 && ( psectlotag == 0 || psectlotag == 2 ) ) k = 32;
                 else k = 16;

                 if(myz > (fz-(k<<8)) )
                            myz += ((fz-(k<<8))-myz)>>1;
                 if(myz < (cz+(18<<8)) )
                            myz = cz+(18<<8);
        }
        else if( psectlotag != 2 )
        {
            if (psectlotag == 1 && p->spritebridge == 0)
            {
                 if(shrunk == 0) i = 34;
                 else i = 12;
            }
                 if(myz < (fz-(i<<8)) && (floorspace(psect)|ceilingspace(psect)) == 0 ) //falling
                 {
                            if( (sb_snum&3) == 0 && !(p->OnMotorcycle || p->OnBoat) && myonground && (sector[psect].floorstat&2) && myz >= (fz-(i<<8)-(16<<8) ) )
                                     myz = fz-(i<<8);
                            else
                            {
                                     myonground = 0;

                                     myzvel += (gc+80);

                                     if(myzvel >= (4096+2048)) myzvel = (4096+2048);
                            }
                 }

                 else
                 {
                            if(psectlotag != 1 && psectlotag != 2 && myonground == 0 && myzvel > (6144>>1))
                                 myhardlanding = myzvel>>10;
                            myonground = 1;

                            if(i==40)
                            {
                                     //Smooth on the ground

                                     k = ((fz-(i<<8))-myz)>>1;
                                     if( klabs(k) < 256 ) k = 0;
                                     myz += k; // ((fz-(i<<8))-myz)>>1;
                                     myzvel -= 768; // 412;
                                     if(myzvel < 0) myzvel = 0;
                            }
                            else if(myjumpingcounter == 0)
                            {
                                myz += ((fz-(i<<7))-myz)>>1; //Smooth on the water
                                if(p->on_warping_sector == 0 && myz > fz-(16<<8))
                                {
                                    myz = fz-(16<<8);
                                    myzvel >>= 1;
                                }
                            }

                            if( (sb_snum&2) && !(p->OnMotorcycle || p->OnBoat) )
                                     myz += (2048+768);

                            if( (sb_snum&1) == 0 && !(p->OnMotorcycle || p->OnBoat) && myjumpingtoggle == 1)
                                     myjumpingtoggle = 0;

                            else if( (sb_snum&1) && !(p->OnMotorcycle || p->OnBoat) && myjumpingtoggle == 0 )
                            {
                                     if( myjumpingcounter == 0 )
                                             if( (fz-cz) > (56<<8) )
                                             {
                                                myjumpingcounter = 1;
                                                myjumpingtoggle = 1;
                                             }
                            }
                            if(!isRR() && myjumpingcounter && (sb_snum&1) == 0 )
                                myjumpingcounter = 0;
                 }

                 if(myjumpingcounter)
                 {
                            if( (sb_snum&1) == 0 && !(p->OnMotorcycle || p->OnBoat) && myjumpingtoggle == 1)
                                     myjumpingtoggle = 0;

                            if( myjumpingcounter < (1024+256) )
                            {
                                     if(psectlotag == 1 && myjumpingcounter > 768)
                                     {
                                             myjumpingcounter = 0;
                                             myzvel = -512;
                                     }
                                     else
                                     {
                                             myzvel -= (sintable[(2048-128+myjumpingcounter)&2047])/12;
                                             myjumpingcounter += 180;

                                             myonground = 0;
                                     }
                            }
                            else
                            {
                                     myjumpingcounter = 0;
                                     myzvel = 0;
                            }
                 }

                 myz += myzvel;

                 if(myz < (cz+(4<<8)) )
                 {
                            myjumpingcounter = 0;
                            if(myzvel < 0) myxvel = myyvel = 0;
                            myzvel = 128;
                            myz = cz+(4<<8);
                 }

        }

        if ( p->fist_incs ||
                     p->transporter_hold > 2 ||
                     myhardlanding ||
                     p->access_incs > 0 ||
                     p->knee_incs > 0 ||
                     (p->curr_weapon == TRIPBOMB_WEAPON &&
                      p->kickback_pic > 1 &&
                      p->kickback_pic < 4 ) )
        {
                 doubvel = 0;
                 myxvel = 0;
                 myyvel = 0;
        }
        else if ( syn->avel )          //p->ang += syncangvel * constant
        {                         //ENGINE calculates angvel for you
            int tempang;

            tempang = syn->avel<<1;

            if(psectlotag == 2)
                myang += (tempang-(tempang>>3))*sgn(doubvel);
            else myang += (tempang)*sgn(doubvel);
            myang &= 2047;
        }

        if ( myxvel || myyvel || syn->fvel || syn->svel )
        {
                 if(p->steroids_amount > 0 && p->steroids_amount < 400)
                     doubvel <<= 1;

                 myxvel += ((syn->fvel*doubvel)<<6);
                 myyvel += ((syn->svel*doubvel)<<6);

                 if( ( p->curr_weapon == KNEE_WEAPON && p->kickback_pic > 10 && myonground ) || ( myonground && (sb_snum&2) && !(p->OnMotorcycle || p->OnBoat)) )
                 {
                            myxvel = mulscale16(myxvel,dukefriction-0x2000);
                            myyvel = mulscale16(myyvel,dukefriction-0x2000);
                 }
                 else
                 {
                    if(psectlotag == 2)
                    {
                        myxvel = mulscale16(myxvel,dukefriction-0x1400);
                        myyvel = mulscale16(myyvel,dukefriction-0x1400);
                    }
                    else
                    {
                        myxvel = mulscale16(myxvel,dukefriction);
                        myyvel = mulscale16(myyvel,dukefriction);
                    }
                 }

                 if( abs(myxvel) < 2048 && abs(myyvel) < 2048 )
                     myxvel = myyvel = 0;

                 if( shrunk )
                 {
                     myxvel =
                         mulscale16(myxvel,(dukefriction)-(dukefriction>>1)+(dukefriction>>2));
                     myyvel =
                         mulscale16(myyvel,(dukefriction)-(dukefriction>>1)+(dukefriction>>2));
                 }
        }

FAKEHORIZONLY:
        if(psectlotag == 1 || spritebridge == 1) i = (4L<<8); else i = (20L<<8);

        clipmove(&myx,&myy,&myz,&mycursectnum,myxvel,myyvel,164L,4L<<8,i,CLIPMASK0);
        pushmove(&myx,&myy,&myz,&mycursectnum,164L,4L<<8,4L<<8,CLIPMASK0);

        if( p->jetpack_on == 0 && psectlotag != 1 && psectlotag != 2 && shrunk)
            myz += 30<<8;

        if ((sb_snum&(1<<18)) || myhardlanding)
            myreturntocenter = 9;

        if (sb_snum&(1<<13))
        {
                myreturntocenter = 9;
                if (sb_snum&(1<<5)) myhoriz += 6;
                myhoriz += 6;
        }
        else if (sb_snum&(1<<14))
        {
                myreturntocenter = 9;
                if (sb_snum&(1<<5)) myhoriz -= 6;
                myhoriz -= 6;
        }
        else if ((sb_snum&(1<<3)) && !(p->OnMotorcycle || p->OnBoat))
        {
                if (sb_snum&(1<<5)) myhoriz += 6;
                myhoriz += 6;
        }
        else if ((sb_snum&(1<<4)) && !(p->OnMotorcycle || p->OnBoat))
        {
                if (sb_snum&(1<<5)) myhoriz -= 6;
                myhoriz -= 6;
        }

        if (myreturntocenter > 0)
            if ((sb_snum&(1<<13)) == 0 && (sb_snum&(1<<14)) == 0)
        {
             myreturntocenter--;
             myhoriz += 33-(myhoriz/3);
        }

        if(p->aim_mode)
            myhoriz += syn->horz>>1;
        else
        {
            if( myhoriz > 95 && myhoriz < 105) myhoriz = 100;
            if( myhorizoff > -5 && myhorizoff < 5) myhorizoff = 0;
        }

        if (myhardlanding > 0)
        {
            myhardlanding--;
            myhoriz -= (myhardlanding<<4);
        }

        if (myhoriz > 299) myhoriz = 299;
        else if (myhoriz < -99) myhoriz = -99;

        if(p->knee_incs > 0)
        {
            myhoriz -= 48;
            myreturntocenter = 9;
        }


ENDFAKEPROCESSINPUT:

        myxbak[fakemovefifoplc&(MOVEFIFOSIZ-1)] = myx;
        myybak[fakemovefifoplc&(MOVEFIFOSIZ-1)] = myy;
        myzbak[fakemovefifoplc&(MOVEFIFOSIZ-1)] = myz;
        myangbak[fakemovefifoplc&(MOVEFIFOSIZ-1)] = myang;
        myhorizbak[fakemovefifoplc&(MOVEFIFOSIZ-1)] = myhoriz;
        fakemovefifoplc++;

        sprite[p->i].cstat = backcstat;
}
#endif

END_DUKE_NS
