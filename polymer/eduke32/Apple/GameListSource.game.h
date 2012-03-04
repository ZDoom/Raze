/*
 *  GameListSource.game.h
 *  duke3d
 *
 *  Created by Jonathon Fowler on 24/07/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

@interface GameListSource : NSObject
{
    NSMutableArray *list;
}
- (id)init;
- (void)dealloc;
- (GrpFile*)grpAtIndex:(int)index;
- (int)findIndexForGrpname:(NSString*)grpname;
- (id)tableView:(NSTableView *)aTableView
objectValueForTableColumn:(NSTableColumn *)aTableColumn
	    row:(int)rowIndex;
- (int)numberOfRowsInTableView:(NSTableView *)aTableView;
@end

