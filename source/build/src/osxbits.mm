#include "compat.h"
#include "osxbits.h"
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#ifndef MAC_OS_X_VERSION_10_5
# define NSImageScaleNone NSScaleNone
#endif

#ifndef MAC_OS_X_VERSION_10_12
# define NSEventModifierFlagOption NSAlternateKeyMask
# define NSEventModifierFlagCommand NSCommandKeyMask
# define NSEventMaskAny NSAnyEventMask
# define NSWindowStyleMaskTitled NSTitledWindowMask
# define NSWindowStyleMaskClosable NSClosableWindowMask
# define NSWindowStyleMaskMiniaturizable NSMiniaturizableWindowMask
# define NSWindowStyleMaskResizable NSResizableWindowMask
# define NSAlertStyleInformational NSInformationalAlertStyle
# define NSControlSizeSmall NSSmallControlSize
#endif

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
	[alert setAlertStyle: NSAlertStyleInformational];

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
	[alert setAlertStyle: NSAlertStyleInformational];

	r = ([alert runModal] == NSAlertFirstButtonReturn);

	[alert release];
#else
	r = (NSRunAlertPanel(nil, mmsg, @"Yes", @"No", nil) == NSAlertDefaultReturn);
#endif

	[mmsg release];

	UNREFERENCED_PARAMETER(name);

	return r;
}

char *osx_gethomedir(void)
{
    NSString *path = NSHomeDirectory();
    const char *Cpath = [path UTF8String];
    char *returnpath = NULL;

    if (Cpath)
        returnpath = Bstrdup(Cpath);

    [path release];

    return returnpath;
}

char *osx_getsupportdir(int32_t local)
{
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, local ? NSUserDomainMask : NSLocalDomainMask, YES);
    char *returnpath = NULL;

    if ([paths count] > 0)
    {
        const char *Cpath = [[paths objectAtIndex:0] UTF8String];

        if (Cpath)
            returnpath = Bstrdup(Cpath);
    }

    [paths release];

    return returnpath;
}

char *osx_getappdir(void)
{
    CFBundleRef mainBundle;
    CFURLRef resUrl, fullUrl;
	CFStringRef str;
    const char *s;
    char *dir = NULL;

    mainBundle = CFBundleGetMainBundle();
    if (!mainBundle) {
        return NULL;
    }

    resUrl = CFBundleCopyResourcesDirectoryURL(mainBundle);
    CFRelease(mainBundle);
    if (!resUrl) {
        return NULL;
    }
    fullUrl = CFURLCopyAbsoluteURL(resUrl);
    if (fullUrl) {
        CFRelease(resUrl);
        resUrl = fullUrl;
    }

	str = CFURLCopyFileSystemPath(resUrl, kCFURLPOSIXPathStyle);
    CFRelease(resUrl);
    if (!str) {
        return NULL;
    }

    s = CFStringGetCStringPtr(str, CFStringGetSystemEncoding());
    if (s) {
        dir = strdup(s);
    }
    CFRelease(str);

    return dir;
}

char *osx_getapplicationsdir(int32_t local)
{
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSAllApplicationsDirectory, local ? NSUserDomainMask : NSLocalDomainMask, YES);
    char *returnpath = NULL;

    if ([paths count] > 0)
    {
        const char *Cpath = [[paths objectAtIndex:0] UTF8String];

        if (Cpath)
            returnpath = Bstrdup(Cpath);
    }

    [paths release];

    return returnpath;
}
