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
- (id)initWithGrpfile:(grpfile_t const *)grpfile
{
    self = [super init];
    if (self) {
        fg = grpfile;
        namestring = [NSString stringWithCString:fg->type->name encoding:NSUTF8StringEncoding];
        [namestring retain];
        grpnamestring = [NSString stringWithCString:fg->filename encoding:NSUTF8StringEncoding];
        [grpnamestring retain];
    }
    return self;
}
- (void)dealloc
{
    [namestring release];
    [grpnamestring release];
    [super dealloc];
}
- (NSString *)name
{
    return namestring;
}
- (NSString *)grpname
{
    return grpnamestring;
}
- (grpfile_t const *)entryptr
{
    return fg;
}
@end
