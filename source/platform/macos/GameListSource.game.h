/*
 *  GameListSource.game.h
 *  duke3d
 *
 *  Created by Jonathon Fowler on 24/07/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

@interface GameListSource : NSObject <NSComboBoxDataSource>
{
    NSMutableArray *list;
}
- (id)init;
- (void)dealloc;
- (GrpFile*)grpAtIndex:(int)index;
- (int)findIndexForGrpname:(NSString*)grpname;
- (id)tableView:(NSTableView *)aTableView
objectValueForTableColumn:(NSTableColumn *)aTableColumn
	    row:(NSInteger)rowIndex;
- (int)numberOfRowsInTableView:(NSTableView *)aTableView;
@end

