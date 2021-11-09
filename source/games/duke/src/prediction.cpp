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
#include "m_argv.h"
#include "mapinfo.h"
#include "texturemanager.h"

BEGIN_DUKE_NS 


int myx, omyx, myxvel, myy, omyy, myyvel, myz, omyz, myzvel;
int globalskillsound;
binangle myang, omyang;
fixedhoriz myhoriz, omyhoriz, myhorizoff, omyhorizoff;
int mycursectnum, myjumpingcounter;
uint8_t myjumpingtoggle, myonground, myhardlanding,myreturntocenter;
int fakemovefifoplc;
int myxbak[MOVEFIFOSIZ], myybak[MOVEFIFOSIZ], myzbak[MOVEFIFOSIZ];
int myhorizbak[MOVEFIFOSIZ];
short myangbak[MOVEFIFOSIZ];


void resetmys()
{
	myx = omyx = ps[myconnectindex].pos.x;
	myy = omyy = ps[myconnectindex].pos.y;
	myz = omyz = ps[myconnectindex].pos.z;
	myxvel = myyvel = myzvel = 0;
	myang = myang = ps[myconnectindex].angle.ang;
	myhoriz = omyhoriz = ps[myconnectindex].horizon.horiz;
	myhorizoff = omyhorizoff = ps[myconnectindex].horizon.horizoff;
	mycursectnum = ps[myconnectindex].cursectnum;
	myjumpingcounter = ps[myconnectindex].jumping_counter;
	myjumpingtoggle = ps[myconnectindex].jumping_toggle;
	myonground = ps[myconnectindex].on_ground;
	myhardlanding = ps[myconnectindex].hard_landing;
}

#if 0 // todo: fix this when networking works again
void fakedomovethingscorrect(void)
{
	 int i;
	 struct player_struct *p;

	 if (numplayers < 2) return;

	 i = ((movefifoplc-1)&(MOVEFIFOSIZ-1));
	 p = &ps[myconnectindex];

	 if (p->pos.x == myxbak[i] && p->pos.y == myybak[i] && p->pos.z == myzbak[i]
		  && p->horiz == myhorizbak[i] && p->ang == myangbak[i]) return;

	 myx = p->pos.x; omyx = p->oposx; myxvel = p->posxv;
	 myy = p->pos.y; omyy = p->oposy; myyvel = p->posyv;
	 myz = p->pos.z; omyz = p->oposz; myzvel = p->poszv;
	 myang = p->ang; omyang = p->oang;
	 mycursectnum = p->cursectnum;
	 myhoriz = p->horiz; omyhoriz = p->ohoriz;
	 myhorizoff = p->horizoff; omyhorizoff = p->ohorizoff;
	 myjumpingcounter = p->jumping_counter;
	 myjumpingtoggle = p->jumping_toggle;
	 myonground = p->on_ground;
	 myhardlanding = p->hard_landing;

	 fakemovefifoplc = movefifoplc;
	 while (fakemovefifoplc < movefifoend[myconnectindex])
		  fakedomovethings();

}

// This still needs fixing for the magic numbers in the input bits
void fakedomovethings(void)
{
		input *syn;
		struct player_struct *p;
		int i, j, k, doubvel, fz, cz, x, y;
		Collision clz, chz;
		int psect, psectlotag, tempsect, backcstat;
		uint8_t shrunk, spritebridge;
		ESyncBits actions;

		syn = (input *)&inputfifo[fakemovefifoplc&(MOVEFIFOSIZ-1)][myconnectindex];

		p = &ps[myconnectindex];

		backcstat = p->GetActor()->s.cstat;
		p->GetActor()->s.cstat &= ~257;

		actions = syn->actions;

		psect = mycursectnum;
		psectlotag = sector[psect].lotag;
		spritebridge = 0;

		shrunk = (p->GetActor()->s.yrepeat < (isRR()? 8 : 32));

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

		getzrange(myx,myy,myz,psect,&cz,chz,&fz,clz,163L,CLIPMASK0);

		j = getflorzofslope(psect,myx,myy);

		if(clz.type == kHitSector && psectlotag == 1 && abs(myz-j) > gs.playerheight+(16<<8) )
			psectlotag = 0;

		if( p->aim_mode == 0 && myonground && psectlotag != 2 && (sector[psect].floorstat&2) )
		{
				x = myx + bcos(myang, -5);
				y = myy + bsin(myang, -5);
				tempsect = psect;
				updatesector(x,y,&tempsect);
				if (tempsect >= 0)
				{
					 k = getflorzofslope(psect,x,y);
					 if (psect == tempsect)
						  myhorizoff += MulScale(j-k,160, 16);
					 else if (abs(getflorzofslope(tempsect,x,y)-k) <= (4<<8))
						  myhorizoff += MulScale(j-k,160, 16);
				}
		}
		if (myhorizoff > 0) myhorizoff -= ((myhorizoff>>3)+1);
		else if (myhorizoff < 0) myhorizoff += (((-myhorizoff)>>3)+1);

		if(chz.type == kHitSprite)
		{
				if (chz.actor->s.statnum == 1 && chz.actor->s.extra >= 0)
				{
					chz.type = kHitNone;
					cz = getceilzofslope(psect,myx,myy);
				}
		}

		if (clz.type == kHitSprite)
		{
				 if ((clz.actor->s.cstat&33) == 33)
				 {
						psectlotag = 0;
						spritebridge = 1;
				 }
				 if(badguy(chz.actor) && chz.actor->s.xrepeat > 24 && abs(p->GetActor()->s.z- chz.actor->s.z) < (84<<8) )
				 {
					j = getangle(chz.actor->s.x-myx, chz.actor->s.y-myy);
					myxvel -= bcos(j, 4);
					myyvel -= bsin(j, 4);
				}
		}

		if( p->GetActor()->s.extra <= 0 )
		{
				 if( psectlotag == 2 )
				 {
							if(p->on_warping_sector == 0)
							{
									 if( abs(myz-fz) > (gs.playerheight>>1))
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

		if(p->on_crane != nullptr) goto FAKEHORIZONLY;

		if(p->angle.spin.asbam() < 0) myang += 128;

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

									 myzvel += (gs.gravity+80);

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
									 if( abs(k) < 256 ) k = 0;
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
											 myzvel -= bsin(128 + myjumpingcounter) / 12;
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

		if (movementBlocked(snum) || myhardlanding)
		{
				 doubvel = 0;
				 myxvel = 0;
				 myyvel = 0;
		}
		else if ( syn->avel )//p->ang += syncangvel * constant
		{                    //ENGINE calculates angvel for you
			int tempang;

			tempang = syn->avel<<1;

			if(psectlotag == 2)
				myang += (tempang-(tempang>>3))*Sgn(doubvel);
			else myang += (tempang)*Sgn(doubvel);
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
							myxvel = MulScale(myxvel,gs.playerfriction-0x2000, 16);
							myyvel = MulScale(myyvel,gs.playerfriction-0x2000, 16);
				 }
				 else
				 {
					if(psectlotag == 2)
					{
						myxvel = MulScale(myxvel,gs.playerfriction-0x1400, 16);
						myyvel = MulScale(myyvel,gs.playerfriction-0x1400, 16);
					}
					else
					{
						myxvel = MulScale(myxvel,gs.playerfriction, 16);
						myyvel = MulScale(myyvel,gs.playerfriction, 16);
					}
				 }

				 if( abs(myxvel) < 2048 && abs(myyvel) < 2048 )
					 myxvel = myyvel = 0;

				 if( shrunk )
				 {
					 myxvel =
						 MulScale(myxvel,(gs.playerfriction)-(gs.playerfriction>>1)+(gs.playerfriction>>2), 16);
					 myyvel =
						 MulScale(myyvel,(gs.playerfriction)-(gs.playerfriction>>1)+(gs.playerfriction>>2), 16);
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

		myhoriz = clamp(myhoriz, HORIZ_MIN, HORIZ_MAX);

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

		p->GetActor()->s.cstat = backcstat;
}
#endif

END_DUKE_NS
