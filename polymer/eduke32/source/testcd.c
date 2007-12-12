//-------------------------------------------------------------------------
/*
Duke Nukem Copyright (C) 1996, 2003 3D Realms Entertainment

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

Replacement cd-auth routine by Jonathon Fowler.
*/
//-------------------------------------------------------------------------

#if (defined(RENDERTYPEWIN) || defined(WIN32)) && !defined(NOCOPYPROTECT)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "compat.h"
#include "baselayer.h"

/*
 * Why even bother doing this? Because I can. Because it proves the futility
 * of it all. Because it's amusing. Who cares?
 */

char testcd(char *fn, int testsiz)
{
    DWORD driveletters;
    int i, fil, dalen;
    char name[32]="x:\\SUPPORT\\",driv[4]="x:\\";

    strcat(name,fn);

    driveletters = GetLogicalDrives();
    for (i=2; i<26; i++)
    {
        if (!(driveletters & (1ul<<i))) continue;
        driv[0] = 'A'+i;
        if (GetDriveType(driv) != DRIVE_CDROM) continue;

        name[0] = 'A'+i;
        fil = Bopen(name,O_RDONLY,S_IREAD);
        if (fil<0) continue;
        dalen = Bfilelength(fil);
        Bclose(fil);

        if (dalen == testsiz)
        {
            initprintf("Copy Protection: Found CDROM in drive %c:\n", driv[0]);
            return 0;
        }
    }

    wm_msgbox("Duke Nukem 3D","Copy Protection: Failed to find CDROM");
    return 1;
}

#else
char testcd(char *fn)
{
    return 0;
}
#endif

