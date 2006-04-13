#include "osxbits.h"
#include <Cocoa/Cocoa.h>

int osx_msgbox(char *name, char *msg)
{
	NSAlert *alert = [[NSAlert alloc] init];
	NSString *mmsg = [[NSString alloc] initWithCString: msg];
	[alert addButtonWithTitle: @"OK"];
	[alert setMessageText: mmsg];
	[alert setAlertStyle: NSInformationalAlertStyle];
	
	[alert runModal];
	
	[alert release];
	[mmsg release];
	
	return 0;
}

int osx_ynbox(char *name, char *msg)
{
	NSAlert *alert = [[NSAlert alloc] init];
	NSString *mmsg = [[NSString alloc] initWithCString: msg];
	int r;

	[alert addButtonWithTitle:@"Yes"];
	[alert addButtonWithTitle:@"No"];
	[alert setMessageText: mmsg];
	[alert setAlertStyle: NSInformationalAlertStyle];
	
	r = ([alert runModal] == NSAlertFirstButtonReturn);

	[alert release];
	[mmsg release];
	
	return r;
}
