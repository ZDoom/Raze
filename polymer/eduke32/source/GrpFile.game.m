/*
 *  GrpFile.game.m
 *  duke3d
 *
 *  Created by Jonathon Fowler on 24/07/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

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
    return [NSString stringWithCString:(fg->name) encoding:NSUTF8StringEncoding];
}
- (struct grpfile *)entryptr
{
    return fg;
}
@end
