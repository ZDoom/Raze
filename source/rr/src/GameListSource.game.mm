/*
 *  GameListSource.game.m
 *  duke3d
 *
 *  Created by Jonathon Fowler on 24/07/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#import <Foundation/Foundation.h>

#include "compat.h"

#import "GrpFile.game.h"
#import "GameListSource.game.h"

@implementation GameListSource
- (id)init
{
    self = [super init];
    if (self) {
        list = [[NSMutableArray alloc] init];

        for (grpfile_t const *p = foundgrps; p; p=p->next) {
            [list addObject:[[GrpFile alloc] initWithGrpfile:p]];
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
    NSUInteger i, listcount = [list count];
    for (i=0; i<listcount; i++) {
        if ([[[list objectAtIndex:i] grpname] isEqual:grpname]) return i;
    }
    return -1;
}

- (id)tableView:(NSTableView *)aTableView
        objectValueForTableColumn:(NSTableColumn *)aTableColumn
            row:(NSInteger)rowIndex
{
    UNREFERENCED_PARAMETER(aTableView);

    NSParameterAssert((NSUInteger)rowIndex < [list count]);
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
    UNREFERENCED_PARAMETER(aTableView);

    return [list count];
}
@end

