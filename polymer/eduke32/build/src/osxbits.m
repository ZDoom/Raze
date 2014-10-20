#include "compat.h"
#include "osxbits.h"
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#ifndef MAC_OS_VERSION_10_3
# define MAC_OS_VERSION_10_3 1030
#endif

int osx_msgbox(const char *name, const char *msg)
{
	NSString *mmsg = [[NSString alloc] initWithUTF8String:msg];

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

	UNREFERENCED_PARAMETER(name);

	return 0;
}

int osx_ynbox(const char *name, const char *msg)
{
	NSString *mmsg = [[NSString alloc] initWithUTF8String:msg];
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

	UNREFERENCED_PARAMETER(name);

	return r;
}
