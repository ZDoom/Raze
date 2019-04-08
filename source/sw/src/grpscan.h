//-------------------------------------------------------------------------
/*
 Copyright (C) 2007 Jonathon Fowler <jf@jonof.id.au>

 This file is part of JFShadowWarrior

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
 */
//-------------------------------------------------------------------------

#ifndef grpscan_h__
#define grpscan_h__

// List of internally-known GRP files
#define numgrpfiles 3
struct internalgrpfile
{
    const char *name;
    unsigned int crcval;
    int size;
};
struct grpfile
{
    char *name;
    unsigned int crcval;
    int size;
    struct grpfile *next;
};

extern internalgrpfile grpfiles[numgrpfiles];
extern grpfile *foundgrps;

int ScanGroups(void);
void FreeGroups(void);

#endif
