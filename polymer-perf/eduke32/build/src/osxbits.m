#include "osxbits.h"
#import <Cocoa/Cocoa.h>

#ifndef MAC_OS_VERSION_10_3
# define MAC_OS_VERSION_10_3 1030
#endif

int osx_msgbox(char *name, char *msg)
{
	NSString *mmsg = [[NSString alloc] initWithCString: msg];

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_3
	NSAlert *alert = [[NSAlert alloc] init];
	[alert addButtonWithTitle: @"OK"];
	[alert setInformativeText: mmsg];
	[alert setAlertStyle: NSInformationalAlertStyle];
	
	[alert runModal];
	
	[alert release];

#else
	NSRunAlertPanel(nil, mmsg, @"OK", nil, nil);
#endif

	[mmsg release];
	return 0;
}

int osx_ynbox(char *name, char *msg)
{
	NSString *mmsg = [[NSString alloc] initWithCString: msg];
	int r;

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_3
	NSAlert *alert = [[NSAlert alloc] init];

	[alert addButtonWithTitle:@"Yes"];
	[alert addButtonWithTitle:@"No"];
	[alert setInformativeText: mmsg];
	[alert setAlertStyle: NSInformationalAlertStyle];
	
	r = ([alert runModal] == NSAlertFirstButtonReturn);

	[alert release];
#else
	r = (NSRunAlertPanel(nil, mmsg, @"Yes", @"No", nil) == NSAlertDefaultReturn);
#endif

	[mmsg release];
	return r;
}
