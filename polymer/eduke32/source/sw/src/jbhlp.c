//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2005 - 3D Realms Entertainment

This file is NOT part of Shadow Warrior version 1.2
However, it is either an older version of a file that is, or is
some test code written during the development of Shadow Warrior.
This file is provided purely for educational interest.

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

Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#include "build.h"
#include "editor.h"
#include "cache1d.h"

#include "keys.h"
#include "names2.h"
#include "game.h"


#define M_RED 12
#define M_BLUE 9


// Globals

static char tempbuf[256];


// Prototypes

void Message(char *string, char color);
long GetAToken(char *name, char *tc, long length);
uint8_t* BKeyPressed(void);
void ResetKeys(void);


// Functions

void Msg(char *string, char color)
{
    clearmidstatbar16();

    printext16(1*4,ydim16+4*8,color,-1,string,1);

}


// @ symbol precedes a tag name target
// # symbol precedes a comment in the help file
long GetAToken(char *name, char *tc, long length)
{
    int i,x=0;
    char t,*tmp,tokenfound=0;
    char token[10];
    long count=0;

    do
    {

        // Find the token symbol
        do
        {
            t = *tc;
            tc++;
            count++;
        }
        while (t!='@' && count < length);


        if (t=='@')
        {
            tmp = token;
            x=1;

            do
            {
                // Read in the token
                *tmp = t;
                tmp++;
                t = *tc;
                tc++;
                x++;
                count++;
            }
            while (t>=48 && t!='@' && x < 9 && count < length);

            *tmp = 0;

            if (!strcmp(name,Bstrupr(token)))
                tokenfound = 1;
        }
    }
    while (!tokenfound && count < length);


    if (!tokenfound) count=0;
    return count;

}

void ContextHelp(short spritenum)
{
    int i,fp,x=0,y=4;
    char t,*tc;
    char name[20];
    char *filebuffer;
    SPRITEp sp;
    short hitag=0;
    long size=0,tokresult=0;


    sp = &sprite[spritenum];

    clearmidstatbar16();

    if ((fp=kopen4load("swbhelp.hlp",0)) == -1)
    {
        Msg("ERROR: Help file not found.",M_RED);
        return;
    }

    // Read in whole file
    size = kfilelength(fp);
    filebuffer = (char *)malloc(size);
    if (filebuffer == NULL)
    {
        Msg("Not enough memory to load swhelp.hlp",M_RED);
        return;
    }

    if (kread(fp, filebuffer, size) != size)
    {
        Msg("Unexpected end of file while reading swhelp.hlp",M_RED);
        kclose(fp);
        return;
    }

    // close the file
    kclose(fp);

    // Conver filebuffer to all upper case
    //strupr(filebuffer);

    // Assign a token name to search for based on the sprite being pointed to.

    // Make the token
    // Make sure 500-600 SOBJ bounding tags all say the same thing.
    hitag = sp->hitag;
    if (hitag > 500 && hitag <= 600) hitag = 500;
    // Give help summary for unknown sprites.
    if ((hitag == 0 || hitag > 1006) && sp->lotag == 0) hitag = 999;

    sprintf(name,"@TAG%d",hitag);

    tc = filebuffer;

    if (!(tokresult = GetAToken(name,tc,size)))
    {
        // This message should never happen unless something is real wrong!
        Msg("No help available.",M_RED);
        return;
    }

    tc += tokresult;

    do
    {
        tc++;
        t = *tc;
        while (t!='\n' && t!='@' && t!='#' && x<128)
        {
            tempbuf[x]=t;
            tc++;
            t = *tc;
            x++;
            if (x >= 128) break;
        }
        tempbuf[x]=0;
        x=0;
        printext16(x*4,ydim16+(y*6)+2,11,-1,tempbuf,1);
        y++;

        if (y>16)
        {
            y=18;
            printext16(x*4,ydim16+(y*6)+2,11,-1,"Hit any key to continue or Q to quit....",1);
            while (BKeyPressed() == NULL) ;
            if (keystatus[KEYSC_Q])
            {
                clearmidstatbar16();
                return;
            }
            ResetKeys();
            clearmidstatbar16();

            y=6;
        }

    }
    while (t!='@' && t!='#');
}


