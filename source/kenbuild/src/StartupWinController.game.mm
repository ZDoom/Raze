#import <Cocoa/Cocoa.h>

#include "compat.h"
#include "baselayer.h"
#include "build.h"

static struct
{
    int fullscreen;
    int xdim3d, ydim3d, bpp3d;
    int forcesetup;
} settings;

@interface StartupWinController : NSWindowController
{
    NSMutableArray *modeslist3d;

    IBOutlet NSButton *alwaysShowButton;
    IBOutlet NSButton *fullscreenButton;
    IBOutlet NSTextView *messagesView;
    IBOutlet NSTabView *tabView;
    IBOutlet NSPopUpButton *videoMode3DPUButton;

    IBOutlet NSButton *cancelButton;
    IBOutlet NSButton *startButton;
}

- (void)dealloc;
- (void)populateVideoModes:(BOOL)firstTime;

- (IBAction)alwaysShowClicked:(id)sender;
- (IBAction)fullscreenClicked:(id)sender;

- (IBAction)cancel:(id)sender;
- (IBAction)start:(id)sender;

- (void)setupRunMode;
- (void)setupMessagesMode;
- (void)putsMessage:(NSString *)str;
- (void)setTitle:(NSString *)str;
@end

@implementation StartupWinController

- (void)dealloc
{
    [modeslist3d release];
    [super dealloc];
}

- (void)populateVideoModes:(BOOL)firstTime
{
    int i, mode3d, fullscreen = ([fullscreenButton state] == NSOnState);
    int idx3d = -1;
    int xdim = 0, ydim = 0, bpp = 0;

    if (firstTime)
    {
        xdim = settings.xdim3d;
        ydim = settings.ydim3d;
        bpp  = settings.bpp3d;
    }
    else
    {
        mode3d = [[modeslist3d objectAtIndex:[videoMode3DPUButton indexOfSelectedItem]] intValue];
        if (mode3d >= 0)
        {
            xdim = validmode[mode3d].xdim;
            ydim = validmode[mode3d].ydim;
            bpp = validmode[mode3d].bpp;
        }
    }

    mode3d = checkvideomode(&xdim, &ydim, bpp, fullscreen, 1);
    if (mode3d < 0)
    {
        int i, cd[] = { 32, 24, 16, 15, 8, 0 };
        for (i=0; cd[i]; ) { if (cd[i] >= bpp) i++; else break; }
        for (; cd[i]; i++)
        {
            mode3d = checkvideomode(&xdim, &ydim, cd[i], fullscreen, 1);
            if (mode3d < 0) continue;
            break;
        }
    }

    [modeslist3d release];
    [videoMode3DPUButton removeAllItems];

    modeslist3d = [[NSMutableArray alloc] init];

    for (i = 0; i < validmodecnt; i++)
    {
        if (fullscreen == validmode[i].fs)
        {
            if (i == mode3d) idx3d = [modeslist3d count];
            [modeslist3d addObject:[NSNumber numberWithInt:i]];
            [videoMode3DPUButton addItemWithTitle:[NSString stringWithFormat:@"%d %C %d %d-bpp",
                                                   validmode[i].xdim, 0xd7, validmode[i].ydim, validmode[i].bpp]];
        }
    }

    if (idx3d >= 0) [videoMode3DPUButton selectItemAtIndex:idx3d];
}

- (IBAction)alwaysShowClicked:(id)sender
{
    UNREFERENCED_PARAMETER(sender);
}

- (IBAction)fullscreenClicked:(id)sender
{
    [self populateVideoModes:NO];

    UNREFERENCED_PARAMETER(sender);
}

- (IBAction)cancel:(id)sender
{
    [NSApp abortModal];

    UNREFERENCED_PARAMETER(sender);
}

- (IBAction)start:(id)sender
{
    int mode = [[modeslist3d objectAtIndex:[videoMode3DPUButton indexOfSelectedItem]] intValue];
    if (mode >= 0)
    {
        settings.xdim3d = validmode[mode].xdim;
        settings.ydim3d = validmode[mode].ydim;
        settings.bpp3d = validmode[mode].bpp;
        settings.fullscreen = validmode[mode].fs;
    }

    settings.forcesetup = [alwaysShowButton state] == NSOnState;

    [NSApp stopModal];

    UNREFERENCED_PARAMETER(sender);
}

- (void)setupRunMode
{
    getvalidmodes();

    [fullscreenButton setState: (settings.fullscreen ? NSOnState : NSOffState)];
    [alwaysShowButton setState: (settings.forcesetup ? NSOnState : NSOffState)];
    [self populateVideoModes:YES];

    // enable all the controls on the Configuration page
    NSEnumerator *enumerator = [[[[tabView tabViewItemAtIndex:0] view] subviews] objectEnumerator];
    NSControl *control;
    while (control = [enumerator nextObject]) [control setEnabled:true];

    [cancelButton setEnabled:true];
    [startButton setEnabled:true];

    [tabView selectTabViewItemAtIndex:0];
    [NSCursor unhide];  // Why should I need to do this?
}

- (void)setupMessagesMode
{
    [tabView selectTabViewItemAtIndex:1];

    // disable all the controls on the Configuration page except "always show", so the
    // user can enable it if they want to while waiting for something else to happen
    NSEnumerator *enumerator = [[[[tabView tabViewItemAtIndex:0] view] subviews] objectEnumerator];
    NSControl *control;
    while (control = [enumerator nextObject])
    {
        if (control == alwaysShowButton) continue;
        [control setEnabled:false];
    }

    [cancelButton setEnabled:false];
    [startButton setEnabled:false];
}

- (void)putsMessage:(NSString *)str
{
    NSRange end;
    NSTextStorage *text = [messagesView textStorage];
    BOOL shouldAutoScroll;

    shouldAutoScroll = ((int)NSMaxY([messagesView bounds]) == (int)NSMaxY([messagesView visibleRect]));

    end.location = [text length];
    end.length = 0;

    [text beginEditing];
    [messagesView replaceCharactersInRange:end withString:str];
    [text endEditing];

    if (shouldAutoScroll)
    {
        end.location = [text length];
        end.length = 0;
        [messagesView scrollRangeToVisible:end];
    }
}

- (void)setTitle:(NSString *)str
{
    [[self window] setTitle:str];
}

@end

static StartupWinController *startwin = nil;

int startwin_open(void)
{
    if (startwin != nil) return 1;

    startwin = [[StartupWinController alloc] initWithWindowNibName:@"startwin.game"];
    if (startwin == nil) return -1;

    [startwin setupMessagesMode];
    [startwin showWindow:nil];

    return 0;
}

int startwin_close(void)
{
    if (startwin == nil) return 1;

    [startwin close];
    [startwin release];
    startwin = nil;

    return 0;
}

int startwin_puts(const char *s)
{
    NSString *ns;

    if (!s) return -1;
    if (startwin == nil) return 1;

    ns = [[NSString alloc] initWithUTF8String:s];
    [startwin putsMessage:ns];
    [ns release];

    return 0;
}

int startwin_settitle(const char *s)
{
    NSString *ns;

    if (!s) return -1;
    if (startwin == nil) return 1;

    ns = [[NSString alloc] initWithUTF8String:s];
    [startwin setTitle:ns];
    [ns release];

    return 0;
}

int startwin_idle(void *v)
{
    if (startwin) [[startwin window] displayIfNeeded];
    UNREFERENCED_PARAMETER(v);
    return 0;
}

extern int xdimgame, ydimgame, bppgame, forcesetup;

int startwin_run(void)
{
    int retval;

    if (startwin == nil) return 0;

    settings.fullscreen = fullscreen;
    settings.xdim3d = xdimgame;
    settings.ydim3d = ydimgame;
    settings.bpp3d = bppgame;
    settings.forcesetup = forcesetup;

    [startwin setupRunMode];

    switch ([NSApp runModalForWindow:[startwin window]])
    {
#ifdef MAC_OS_X_VERSION_10_9
    case NSModalResponseStop: retval = 1; break;
    case NSModalResponseAbort: retval = 0; break;
#else
    case NSRunStoppedResponse: retval = 1; break;
    case NSRunAbortedResponse: retval = 0; break;
#endif
    default: retval = -1;
    }

    [startwin setupMessagesMode];

    if (retval)
    {
        fullscreen = settings.fullscreen;
        xdimgame = settings.xdim3d;
        ydimgame = settings.ydim3d;
        bppgame = settings.bpp3d;
        forcesetup = settings.forcesetup;
    }

    return retval;
}
