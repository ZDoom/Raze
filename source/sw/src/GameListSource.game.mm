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


#import <Cocoa/Cocoa.h>

#import "GrpFile.game.h"
#import "GameListSource.game.h"

@implementation GameListSource
- (id)init
{
    self = [super init];
    if (self) {
        struct grpfile *p;
        int i;

        list = [[NSMutableArray alloc] init];

        for (p = foundgrps; p; p=p->next) {
            for (i=0; i<numgrpfiles; i++) if (p->crcval == grpfiles[i].crcval) break;
            if (i == numgrpfiles) continue;
            [list addObject:[[GrpFile alloc] initWithGrpfile:p andName:[NSString stringWithUTF8String:grpfiles[i].name]]];
        }
    }

    return self;
}

- (void)dealloc
{
    [list release];
    [super dealloc];
}

- (GrpFile*)grpAtIndex:(int)index
{
    return [list objectAtIndex:index];
}

- (int)findIndexForGrpname:(NSString*)grpname
{
    unsigned i;
    for (i=0; i<[list count]; i++) {
        if ([[[list objectAtIndex:i] grpname] isEqual:grpname]) return i;
    }
    return -1;
}

- (id)tableView:(NSTableView *)aTableView
        objectValueForTableColumn:(NSTableColumn *)aTableColumn
            row:(NSInteger)rowIndex
{
    NSParameterAssert(rowIndex >= 0 && rowIndex < [list count]);
    switch ([[aTableColumn identifier] intValue]) {
        case 0:	// name column
            return [[list objectAtIndex:rowIndex] name];
        case 1:	// grp column
            return [[list objectAtIndex:rowIndex] grpname];
        default: return nil;
    }
}

- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
    return [list count];
}
@end

