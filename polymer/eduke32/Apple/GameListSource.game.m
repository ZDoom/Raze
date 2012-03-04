/*
 *  GameListSource.game.m
 *  duke3d
 *
 *  Created by Jonathon Fowler on 24/07/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

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
            [list addObject:[[GrpFile alloc] initWithGrpfile:p andName:[NSString stringWithCString:grpfiles[i].name]]];
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
    int i;
    for (i=0; i<[list count]; i++) {
        if ([[[list objectAtIndex:i] grpname] isEqual:grpname]) return i;
    }
    return -1;
}

- (id)tableView:(NSTableView *)aTableView
        objectValueForTableColumn:(NSTableColumn *)aTableColumn
            row:(int)rowIndex
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

