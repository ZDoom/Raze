
#import <Cocoa/Cocoa.h>

#include "compat.h"
#include "baselayer.h"
#include "build.h"
#include "editor.h"

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

static NSRect NSRectChangeXY(NSRect const rect, CGFloat const x, CGFloat const y)
{
    return NSMakeRect(x, y, rect.size.width, rect.size.height);
}
static NSRect NSSizeAddXY(NSSize const size, CGFloat const x, CGFloat const y)
{
    return NSMakeRect(x, y, size.width, size.height);
}
#if 0
static CGFloat NSRightEdge(NSRect rect)
{
    return rect.origin.x + rect.size.width;
}
static CGFloat NSTopEdge(NSRect rect)
{
    return rect.origin.y + rect.size.height;
}
#endif

static void setFontToSmall(id control)
{
    [control setFont:[NSFont fontWithDescriptor:[[control font] fontDescriptor] size:[NSFont smallSystemFontSize]]];
}

static void setControlToSmall(id control)
{
    [control setControlSize:NSControlSizeSmall];
}

static NSTextField * makeLabel(NSString * labelText)
{
    NSTextField *textField = [[NSTextField alloc] init];
    setFontToSmall(textField);
    setControlToSmall([textField cell]);
    [textField setStringValue:labelText];
    [textField setBezeled:NO];
    [textField setDrawsBackground:NO];
    [textField setEditable:NO];
    [textField setSelectable:NO];
    [textField sizeToFit];
    return textField;
}

static NSButton * makeCheckbox(NSString * labelText)
{
    NSButton *checkbox = [[NSButton alloc] init];
    setFontToSmall(checkbox);
    setControlToSmall([checkbox cell]);
    [checkbox setTitle:labelText];
    [checkbox setButtonType:NSSwitchButton];
    [checkbox sizeToFit];
    return checkbox;
}

static NSPopUpButton * makeComboBox(void)
{
    NSPopUpButton *comboBox = [[NSPopUpButton alloc] init];
    [comboBox setPullsDown:NO];
    setFontToSmall(comboBox);
    setControlToSmall([comboBox cell]);
    [comboBox setBezelStyle:NSRoundedBezelStyle];
    [comboBox setPreferredEdge:NSMaxYEdge];
    [[comboBox cell] setArrowPosition:NSPopUpArrowAtCenter];
    [comboBox sizeToFit];
    return comboBox;
}

static id nsapp;

/* setAppleMenu disappeared from the headers in 10.4 */
@interface NSApplication(NSAppleMenu)
- (void)setAppleMenu:(NSMenu *)menu;
@end

static NSString * GetApplicationName(void)
{
    NSString *appName = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleDisplayName"];
    if (!appName)
        appName = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleName"];
    if (![appName length])
        appName = [[NSProcessInfo processInfo] processName];

    return appName;
}

static void CreateApplicationMenus(void)
{
    NSString *appName;
    NSString *title;
    NSMenu *rootMenu;
    NSMenu *serviceMenu;
    NSMenuItem *menuItem;

    NSMenu *mainMenu = [[NSMenu alloc] init];

    /* Create the application menu */
    appName = GetApplicationName();
    rootMenu = [[NSMenu alloc] init];

    /* Put menu into the menubar */
    menuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    [menuItem setSubmenu:rootMenu];
    [mainMenu addItem:menuItem];
    [menuItem release];

    /* Add menu items */
    title = [@"About " stringByAppendingString:appName];
    [rootMenu addItemWithTitle:title action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];

    [rootMenu addItem:[NSMenuItem separatorItem]];

    serviceMenu = [[NSMenu alloc] init];
    menuItem = (NSMenuItem *)[rootMenu addItemWithTitle:@"Services" action:nil keyEquivalent:@""];
    [menuItem setSubmenu:serviceMenu];

    [nsapp setServicesMenu:serviceMenu];
    [serviceMenu release];

    [rootMenu addItem:[NSMenuItem separatorItem]];

    title = [@"Hide " stringByAppendingString:appName];
    [rootMenu addItemWithTitle:title action:@selector(hide:) keyEquivalent:@"h"];

    menuItem = (NSMenuItem *)[rootMenu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
    [menuItem setKeyEquivalentModifierMask:(NSEventModifierFlagOption|NSEventModifierFlagCommand)];

    [rootMenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];

    [rootMenu addItem:[NSMenuItem separatorItem]];

    title = [@"Quit " stringByAppendingString:appName];
    [rootMenu addItemWithTitle:title action:@selector(terminate:) keyEquivalent:@"q"];

    /* Create the main menu bar */
    [nsapp setMainMenu:mainMenu];
    [mainMenu release];  /* we're done with it, let NSApp own it. */

    /* Tell the application object that this is now the application menu */
    [nsapp setAppleMenu:rootMenu];
    [rootMenu release];
}

static int retval = -1;

static struct {
    int fullscreen;
    int xdim2d, ydim2d;
    int xdim3d, ydim3d, bpp3d;
    int forcesetup;
} settings;

@interface StartupWindow : NSWindow <NSWindowDelegate>
{
    NSMutableArray *modeslist2d;
    NSMutableArray *modeslist3d;

    NSButton *alwaysShowButton;
    NSButton *fullscreenButton;
    NSTextView *messagesView;
    NSTabView *tabView;
    NSTabViewItem *tabViewItemSetup;
    NSTabViewItem *tabViewItemMessageLog;
    NSPopUpButton *videoMode2DPUButton;
    NSPopUpButton *videoMode3DPUButton;

    NSButton *cancelButton;
    NSButton *startButton;
}

- (StartupWindow *)init;

- (void)dealloc;
- (void)populateVideoModes:(BOOL)firstTime;

- (void)fullscreenClicked:(id)sender;

- (void)cancel:(id)sender;
- (void)start:(id)sender;

- (void)setupRunMode;
- (void)setupMessagesMode;

- (void)putsMessage:(NSString *)str;

@end

@implementation StartupWindow : NSWindow

- (StartupWindow *)init
{
    NSUInteger const style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    NSRect const windowFrame = NSMakeRect(0, 0, 480, 280);
    self = [super initWithContentRect:windowFrame styleMask:style backing:NSBackingStoreBuffered defer:NO];

    if (self)
    {
        // window properties
        [self setDelegate:self];
        [self setReleasedWhenClosed:NO];
#if defined MAC_OS_X_VERSION_10_3 && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_3
        [self setContentMinSize:[[self contentView] frame].size];
#else
        [self setMinSize:[NSWindow frameRectForContentRect:[[self contentView] frame] styleMask:[self styleMask]].size];
#endif


        // image on the left
        NSRect const imageFrame = NSMakeRect(0, 0, 100, 280);
        NSImageView * imageView = [[NSImageView alloc] initWithFrame:imageFrame];
        [imageView setImageScaling:NSImageScaleNone];
        [imageView setImage:[NSImage imageNamed:@"build"]];
        [[self contentView] addSubview:imageView];
        [imageView setAutoresizingMask:NSViewMaxXMargin | NSViewHeightSizable];


        // buttons
        CGFloat const buttonWidth = 80;
        CGFloat const buttonHeight = 32;

        NSRect const startButtonFrame = NSMakeRect(windowFrame.size.width - buttonWidth, 0, buttonWidth, buttonHeight);
        startButton = [[NSButton alloc] initWithFrame:startButtonFrame];
        [[self contentView] addSubview:startButton];
        [startButton setTitle:@"Start"];
        [startButton setTarget:self];
        [startButton setAction:@selector(start:)];
        [startButton setBezelStyle:NSRoundedBezelStyle];
        [startButton setKeyEquivalent:@"\r"];
        [startButton setAutoresizingMask:NSViewMinXMargin | NSViewMaxYMargin];

        NSRect const cancelButtonFrame = NSMakeRect(startButtonFrame.origin.x - buttonWidth, 0, buttonWidth, buttonHeight);
        cancelButton = [[NSButton alloc] initWithFrame:cancelButtonFrame];
        [[self contentView] addSubview:cancelButton];
        [cancelButton setTitle:@"Cancel"];
        [cancelButton setTarget:self];
        [cancelButton setAction:@selector(cancel:)];
        [cancelButton setBezelStyle:NSRoundedBezelStyle];
        [cancelButton setAutoresizingMask:NSViewMinXMargin | NSViewMaxYMargin];


        // tab frame
        NSRect const tabViewFrame = NSMakeRect(imageFrame.size.width, buttonHeight, windowFrame.size.width - imageFrame.size.width, windowFrame.size.height - buttonHeight - 5);
        tabView = [[NSTabView alloc] initWithFrame:tabViewFrame];
        [[self contentView] addSubview:tabView];
        setFontToSmall(tabView);
        setControlToSmall(tabView);
        [tabView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];


        // setup tab

        tabViewItemSetup = [[NSTabViewItem alloc] init];
        [tabView addTabViewItem:tabViewItemSetup];
        [tabViewItemSetup setLabel:@"Setup"];
        NSRect const tabViewItemSetupFrame = [[tabViewItemSetup view] frame];


        // always show checkbox
        alwaysShowButton = makeCheckbox(@"Always show this window at startup");
        [[tabViewItemSetup view] addSubview:alwaysShowButton];
        NSSize const alwaysShowButtonSize = [alwaysShowButton frame].size;
        NSRect const alwaysShowButtonFrame = NSSizeAddXY(alwaysShowButtonSize, tabViewItemSetupFrame.size.width - alwaysShowButtonSize.width, 0);
        [alwaysShowButton setFrame:alwaysShowButtonFrame];
        [alwaysShowButton setAutoresizingMask:NSViewMinXMargin | NSViewMaxYMargin];


        // video mode selectors and labels
        NSTextField * label2DVideoMode = makeLabel(@"2D Video mode:");
        [[tabViewItemSetup view] addSubview:label2DVideoMode];
        NSSize const label2DVideoModeSize = [label2DVideoMode frame].size;
        [label2DVideoMode setAutoresizingMask:NSViewMaxXMargin | NSViewMinYMargin];

        NSTextField * label3DVideoMode = makeLabel(@"3D Video mode:");
        [[tabViewItemSetup view] addSubview:label3DVideoMode];
        NSSize const label3DVideoModeSize = [label3DVideoMode frame].size;
        [label3DVideoMode setAutoresizingMask:NSViewMaxXMargin | NSViewMinYMargin];

        fullscreenButton = makeCheckbox(@"Fullscreen");
        [[tabViewItemSetup view] addSubview:fullscreenButton];
        NSSize const fullscreenButtonSize = [fullscreenButton frame].size;
        [fullscreenButton setAction:@selector(fullscreenClicked:)];
        [fullscreenButton setAutoresizingMask:NSViewMinXMargin | NSViewMinYMargin];

        CGFloat const labelsVideoModeRightEdge = max(label2DVideoModeSize.width, label3DVideoModeSize.width);

        videoMode2DPUButton = makeComboBox();
        [[tabViewItemSetup view] addSubview:videoMode2DPUButton];
        NSSize const videoMode2DPUButtonSize = [videoMode2DPUButton frame].size;
        CGFloat const videoMode2DButtonX = labelsVideoModeRightEdge;
        NSRect const videoMode2DPUButtonFrame = NSMakeRect(videoMode2DButtonX, tabViewItemSetupFrame.size.height - videoMode2DPUButtonSize.height, tabViewItemSetupFrame.size.width - videoMode2DButtonX - fullscreenButtonSize.width, videoMode2DPUButtonSize.height);
        [videoMode2DPUButton setFrame:videoMode2DPUButtonFrame];
        [videoMode2DPUButton setAutoresizingMask:NSViewWidthSizable | NSViewMinYMargin];

        videoMode3DPUButton = makeComboBox();
        [[tabViewItemSetup view] addSubview:videoMode3DPUButton];
        NSSize const videoMode3DPUButtonSize = [videoMode3DPUButton frame].size;
        CGFloat const videoMode3DButtonX = labelsVideoModeRightEdge;
        NSRect const videoMode3DPUButtonFrame = NSMakeRect(videoMode3DButtonX, videoMode2DPUButtonFrame.origin.y - videoMode3DPUButtonSize.height, tabViewItemSetupFrame.size.width - videoMode3DButtonX - fullscreenButtonSize.width, videoMode3DPUButtonSize.height);
        [videoMode3DPUButton setFrame:videoMode3DPUButtonFrame];
        [videoMode3DPUButton setAutoresizingMask:NSViewWidthSizable | NSViewMinYMargin];

        NSRect const label2DVideoModeFrame = NSSizeAddXY(label2DVideoModeSize, 0, videoMode2DPUButtonFrame.origin.y + rintf((videoMode2DPUButtonSize.height - label2DVideoModeSize.height) * 0.5f) + 1);
        [label2DVideoMode setFrame:label2DVideoModeFrame];

        NSRect const label3DVideoModeFrame = NSSizeAddXY(label3DVideoModeSize, 0, videoMode3DPUButtonFrame.origin.y + rintf((videoMode3DPUButtonSize.height - label3DVideoModeSize.height) * 0.5f) + 1);
        [label3DVideoMode setFrame:label3DVideoModeFrame];

        NSRect const fullscreenButtonFrame = NSSizeAddXY(fullscreenButtonSize, tabViewItemSetupFrame.size.width - fullscreenButtonSize.width, videoMode3DPUButtonFrame.origin.y + rintf((videoMode3DPUButtonSize.height - fullscreenButtonSize.height) * 0.5f) + 1);
        [fullscreenButton setFrame:fullscreenButtonFrame];


        // message log tab

        tabViewItemMessageLog = [[NSTabViewItem alloc] init];
        [tabView addTabViewItem:tabViewItemMessageLog];
        [tabViewItemMessageLog setLabel:@"Message Log"];
        NSRect const tabViewItemMessageLogFrame = [[tabViewItemMessageLog view] frame];


        // message log
        NSScrollView * messagesScrollView = [[NSScrollView alloc] initWithFrame:NSRectChangeXY(tabViewItemMessageLogFrame, 0, 0)];
        [[tabViewItemMessageLog view] addSubview:messagesScrollView];
        [messagesScrollView setBorderType:NSBezelBorder];
        [messagesScrollView setHasVerticalScroller:YES];
        [messagesScrollView setHasHorizontalScroller:NO];
        setControlToSmall([[messagesScrollView verticalScroller] cell]);
        NSSize const messagesScrollViewContentSize = [messagesScrollView contentSize];
        [messagesScrollView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

        messagesView = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, messagesScrollViewContentSize.width, messagesScrollViewContentSize.height)];
        [messagesScrollView setDocumentView:messagesView];
        [messagesView setEditable:NO];
        [messagesView setRichText:NO];
        setFontToSmall(messagesView);
        [messagesView setMinSize:NSMakeSize(0.0, messagesScrollViewContentSize.height)];
        [messagesView setMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];
        [messagesView setVerticallyResizable:YES];
        [messagesView setHorizontallyResizable:NO];
        [messagesView setAutoresizingMask:NSViewWidthSizable];

        [[messagesView textContainer] setContainerSize:NSMakeSize(messagesScrollViewContentSize.width, FLT_MAX)];
        [[messagesView textContainer] setWidthTracksTextView:YES];
    }

    return self;
}

- (BOOL)canBecomeKeyWindow
{
    return YES;
}

- (BOOL)canBecomeMainWindow
{
    return YES;
}

- (BOOL) windowShouldClose:(id)sender
{
    UNREFERENCED_PARAMETER(sender);

    retval = 0;

    return YES;
}

- (void)dealloc
{
    [modeslist3d release];
    [super dealloc];
}

- (void)populateVideoModes:(BOOL)firstTime
{
    int i, mode3d, fullscreen = ([fullscreenButton state] == NSOnState);
    int mode2d, idx2d = -1;
    int idx3d = -1;
    int xdim2d = 0, ydim2d = 0;
    int const bpp2d = 8;
    int xdim = 0, ydim = 0, bpp = 0;

    if (firstTime) {
        xdim2d = settings.xdim2d;
        ydim2d = settings.ydim2d;
        xdim = settings.xdim3d;
        ydim = settings.ydim3d;
        bpp  = settings.bpp3d;
    } else {
        mode2d = [[modeslist2d objectAtIndex:[videoMode2DPUButton indexOfSelectedItem]] intValue];
        if (mode2d >= 0) {
            xdim2d = validmode[mode2d].xdim;
            ydim2d = validmode[mode2d].ydim;
        }

        mode3d = [[modeslist3d objectAtIndex:[videoMode3DPUButton indexOfSelectedItem]] intValue];
        if (mode3d >= 0) {
            xdim = validmode[mode3d].xdim;
            ydim = validmode[mode3d].ydim;
            bpp = validmode[mode3d].bpp;
        }
    }


    mode2d = checkvideomode(&xdim2d, &ydim2d, bpp2d, fullscreen, 1);

    [modeslist2d release];
    [videoMode2DPUButton removeAllItems];

    modeslist2d = [[NSMutableArray alloc] init];

    for (i = 0; i < validmodecnt; i++) {
        if (fullscreen == validmode[i].fs) {
            if (i == mode2d) idx2d = [modeslist2d count];
            [modeslist2d addObject:[NSNumber numberWithInt:i]];
            [videoMode2DPUButton addItemWithTitle:[NSString stringWithFormat:@"%d %C %d",
                                                   validmode[i].xdim, 0xd7, validmode[i].ydim]];
        }
    }

    if (idx2d >= 0) [videoMode2DPUButton selectItemAtIndex:idx2d];


    mode3d = checkvideomode(&xdim, &ydim, bpp, fullscreen, 1);
    if (mode3d < 0) {
        int i, cd[] = { 32, 24, 16, 15, 8, 0 };
        for (i=0; cd[i]; ) { if (cd[i] >= bpp) i++; else break; }
        for ( ; cd[i]; i++) {
            mode3d = checkvideomode(&xdim, &ydim, cd[i], fullscreen, 1);
            if (mode3d < 0) continue;
            break;
        }
    }

    [modeslist3d release];
    [videoMode3DPUButton removeAllItems];

    modeslist3d = [[NSMutableArray alloc] init];

    for (i = 0; i < validmodecnt; i++) {
        if (fullscreen == validmode[i].fs) {
            if (i == mode3d) idx3d = [modeslist3d count];
            [modeslist3d addObject:[NSNumber numberWithInt:i]];
            [videoMode3DPUButton addItemWithTitle:[NSString stringWithFormat:@"%d %C %d %d-bpp",
                                                   validmode[i].xdim, 0xd7, validmode[i].ydim, validmode[i].bpp]];
        }
    }

    if (idx3d >= 0) [videoMode3DPUButton selectItemAtIndex:idx3d];
}

- (void)fullscreenClicked:(id)sender
{
    UNREFERENCED_PARAMETER(sender);

    [self populateVideoModes:NO];
}

- (void)cancel:(id)sender
{
    UNREFERENCED_PARAMETER(sender);

    retval = 0;
}

- (void)start:(id)sender
{
    UNREFERENCED_PARAMETER(sender);

    int mode2d = [[modeslist2d objectAtIndex:[videoMode2DPUButton indexOfSelectedItem]] intValue];
    if (mode2d >= 0) {
        settings.xdim2d = validmode[mode2d].xdim;
        settings.ydim2d = validmode[mode2d].ydim;
        settings.fullscreen = validmode[mode2d].fs;
    }

    int mode = [[modeslist3d objectAtIndex:[videoMode3DPUButton indexOfSelectedItem]] intValue];
    if (mode >= 0) {
        settings.xdim3d = validmode[mode].xdim;
        settings.ydim3d = validmode[mode].ydim;
        settings.bpp3d = validmode[mode].bpp;
        settings.fullscreen = validmode[mode].fs;
    }

    settings.forcesetup = [alwaysShowButton state] == NSOnState;

    retval = 1;
}

- (void)setupRunMode
{
    getvalidmodes();

    [fullscreenButton setState: (settings.fullscreen ? NSOnState : NSOffState)];
    [alwaysShowButton setState: (settings.forcesetup ? NSOnState : NSOffState)];
    [self populateVideoModes:YES];

    // enable all the controls on the Configuration page
    NSEnumerator *enumerator = [[[tabViewItemSetup view] subviews] objectEnumerator];
    NSControl *control;
    while ((control = [enumerator nextObject]))
    {
        if ([control respondsToSelector:@selector(setEnabled:)])
            [control setEnabled:true];
    }

    [cancelButton setEnabled:true];
    [startButton setEnabled:true];

    [tabView selectTabViewItem:tabViewItemSetup];
    [NSCursor unhide]; // Why should I need to do this?
}

- (void)setupMessagesMode
{
    [tabView selectTabViewItem:tabViewItemMessageLog];

    // disable all the controls on the Configuration page except "always show", so the
    // user can enable it if they want to while waiting for something else to happen
    NSEnumerator *enumerator = [[[tabViewItemSetup view] subviews] objectEnumerator];
    NSControl *control;
    while ((control = [enumerator nextObject]))
    {
        if (control != alwaysShowButton && [control respondsToSelector:@selector(setEnabled:)])
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

    if (shouldAutoScroll) {
        end.location = [text length];
        end.length = 0;
        [messagesView scrollRangeToVisible:end];
    }
}

@end

static StartupWindow *startwin = nil;

int startwin_open(void)
{
    // fix for "ld: absolute address to symbol _NSApp in a different linkage unit not supported"
    // (OS X 10.6) when building for PPC
    nsapp = [NSApplication sharedApplication];

    if (startwin != nil) return 1;

    startwin = [[StartupWindow alloc] init];
    if (startwin == nil) return -1;

    [startwin setupMessagesMode];

    [nsapp finishLaunching];

    [startwin center];
    [startwin makeKeyAndOrderFront:nil];

    CreateApplicationMenus();

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

    ns = [NSString stringWithUTF8String:s];
    [startwin putsMessage:ns];
    [ns release];

    return 0;
}

int startwin_settitle(const char *s)
{
    NSString *ns;

    if (!s) return -1;
    if (startwin == nil) return 1;

    ns = [NSString stringWithUTF8String:s];
    [startwin setTitle:ns];
    [ns release];

    return 0;
}

int startwin_idle(void *v)
{
    UNREFERENCED_PARAMETER(v);

    if (startwin)
    {
        NSEvent *event;
        do
        {
            event = [nsapp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate date] inMode:NSDefaultRunLoopMode dequeue:YES];
            [nsapp sendEvent:event];
        }
        while (event != nil);

        [startwin displayIfNeeded];
        [nsapp updateWindows];
    }

    return 0;
}


int startwin_run(void)
{
    if (startwin == nil) return 0;

    settings.fullscreen = fullscreen;
    settings.xdim2d = xdim2d;
    settings.ydim2d = ydim2d;
    settings.xdim3d = xdimgame;
    settings.ydim3d = ydimgame;
    settings.bpp3d = bppgame;
    settings.forcesetup = forcesetup;

    [startwin setupRunMode];

    do
    {
        NSEvent *event = [nsapp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantFuture] inMode:NSDefaultRunLoopMode dequeue:YES];
        [nsapp sendEvent:event];
        [nsapp updateWindows];
    }
    while (retval == -1);

    [startwin setupMessagesMode];
    [nsapp updateWindows];

    if (retval) {
        fullscreen = settings.fullscreen;
        xdim2d = settings.xdim2d;
        ydim2d = settings.ydim2d;
        xdimgame = settings.xdim3d;
        ydimgame = settings.ydim3d;
        bppgame = settings.bpp3d;
        forcesetup = settings.forcesetup;
    }

    return retval;
}
