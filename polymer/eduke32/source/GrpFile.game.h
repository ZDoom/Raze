/*
 *  GrpFile.game.h
 *  duke3d
 *
 *  Created by Jonathon Fowler on 24/07/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#import <Cocoa/Cocoa.h>

#include "grpscan.h"

@interface GrpFile : NSObject
{
    NSString *name;
    struct grpfile *fg;
}
- (id)initWithGrpfile:(struct grpfile *)grpfile andName:(NSString*)aName;
- (void)dealloc;
- (NSString *)name;
- (NSString *)grpname;
- (struct grpfile *)entryptr;
@end

