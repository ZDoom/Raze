//-------------------------------------------------------------------------
/*
 Copyright (C) 2013 Jonathon Fowler <jf@jonof.id.au>

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

#include "GrpFile.game.h"

@implementation GrpFile
- (id)initWithGrpfile:(struct grpfile *)grpfile andName:(NSString*)aName
{
    self = [super init];
    if (self) {
        fg = grpfile;
        name = aName;
        [aName retain];
    }
    return self;
}
- (void)dealloc
{
    [name release];
    [super dealloc];
}
- (NSString *)name
{
    return name;
}
- (NSString *)grpname
{
    return [NSString stringWithUTF8String:(fg->name)];
}
- (struct grpfile *)entryptr
{
    return fg;
}
@end
