//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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

#include "save.h"
#include <stdio.h>
//#include <fcntl.h>
//#include <sys/stat.h>
//#include <io.h>
#include "engine.h"

int savegame(int nSlot)
{
    char filename[92];

    if (nSlot < 0 || nSlot >= 10) {
        return 0;
    }

    sprintf(filename, "save%1d.gam", nSlot);

    int hFile = open(filename, 609, 128);
    if (hFile != -1)
    {
#if 0 // TODO
        write(hFile, &numsectors, sizeof(numsectors));
        write(hFile, sector, sizeof(SECTOR) * numsectors);
        write(hFile, &numwalls, sizeof(numwalls));
        write(hFile, wall, sizeof(WALL) * numwalls);
        write(hFile, sprite, sizeof(SPRITE) * kMaxSprites);
        write(hFile, headspritesect, sizeof(headspritesect));
        write(hFile, prevspritesect, sizeof(prevspritesect));
        write(hFile, nextspritesect, sizeof(nextspritesect));
        write(hFile, headspritestat, sizeof(headspritestat));
        write(hFile, prevspritestat, sizeof(prevspritestat));
        write(hFile, nextspritestat, sizeof(nextspritestat));
        write(hFile, startumost, sizeof(startumost));
        write(hFile, startdmost, sizeof(startdmost));
        write(hFile, &brightness, 2);
        write(hFile, &visibility, 4);
        write(hFile, &parallaxtype, 1);
        write(hFile, &parallaxyoffs, 4);
        write(hFile, pskyoff, 512);
        write(hFile, &pskybits, 2);
        write(hFile, &inita, 2);
        write(hFile, &initsect, 2);
        write(hFile, &initx, 4);
        write(hFile, &inity, 4);
        write(hFile, &initz, 4);
        write(hFile, &levelnum, 2);
#endif
        close(hFile);
    }

    return 1; // CHECKME
}

int loadgame(int nSlot)
{
    char filename[92];

    if (nSlot < 0 || nSlot >= 10) {
        return 0;
    }

    sprintf(filename, "save%1d.gam", nSlot);

    int hFile = open(filename, 514, 256);
    if (hFile != -1)
    {
#if 0 // TODO
        read(hFile, &numsectors, sizeof(numsectors));
        read(hFile, sector, sizeof(SECTOR) * numsectors);
        read(hFile, &numwalls, sizeof(numwalls));
        read(hFile, wall, sizeof(WALL) * numwalls);
        read(hFile, sprite, sizeof(SPRITE) * kMaxSprites);
        read(hFile, headspritesect, sizeof(headspritesect));
        read(hFile, prevspritesect, sizeof(prevspritesect));
        read(hFile, nextspritesect, sizeof(nextspritesect));
        read(hFile, headspritestat, sizeof(headspritestat));
        read(hFile, prevspritestat, sizeof(prevspritestat));
        read(hFile, nextspritestat, sizeof(nextspritestat));
        read(hFile, startumost, sizeof(startumost));
        read(hFile, startdmost, sizeof(startdmost));
        read(hFile, &brightness, 2);
        read(hFile, &visibility, 4);
        read(hFile, &parallaxtype, 1);
        read(hFile, &parallaxyoffs, 4);
        read(hFile, pskyoff, 512);
        read(hFile, &pskybits, 2);
        read(hFile, &inita, 2);
        read(hFile, &initsect, 2);
        read(hFile, &initx, 4);
        read(hFile, &inity, 4);
        read(hFile, &initz, 4);
        read(hFile, &levelnum, 2);

        lPlayerXVel = 0;
        lPlayerYVel = 0;
        nPlayerDAng = 0;
#endif
        close(hFile);
    }

    return 1; // CHECKME
}

