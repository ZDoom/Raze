/*
 *  GrpFile.game.h
 *  duke3d
 *
 *  Created by Jonathon Fowler on 24/07/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#import <Foundation/Foundation.h>

#include "grpscan.h"

@interface GrpFile : NSObject
{
    NSString *namestring;
    NSString *grpnamestring;
    grpfile_t const *fg;
}
- (id)initWithGrpfile:(grpfile_t const *)grpfile;
- (void)dealloc;
- (NSString *)name;
- (NSString *)grpname;
- (grpfile_t const *)entryptr;
@end

