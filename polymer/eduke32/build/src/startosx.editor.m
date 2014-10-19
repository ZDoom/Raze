#import <Cocoa/Cocoa.h>

#include "baselayer.h"

static id nsapp;

@interface StartupWinController : NSWindowController
{
    IBOutlet NSButton *alwaysShowButton;
    IBOutlet NSButton *fullscreenButton;
    IBOutlet NSTextView *messagesView;
    IBOutlet NSTabView *tabView;
    IBOutlet NSComboBox *videoModeCbox;

    IBOutlet NSButton *cancelButton;
    IBOutlet NSButton *startButton;
}

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

- (IBAction)alwaysShowClicked:(id)sender
{
    UNREFERENCED_PARAMETER(sender);
}

- (IBAction)fullscreenClicked:(id)sender
{
    UNREFERENCED_PARAMETER(sender);

    // XXX: recalculate the video modes list to take into account the fullscreen status
}

- (IBAction)cancel:(id)sender
{
    UNREFERENCED_PARAMETER(sender);

    [nsapp abortModal];
}

- (IBAction)start:(id)sender
{
    UNREFERENCED_PARAMETER(sender);

    // XXX: write the states of the form controls to their respective homes
    [nsapp stopModal];
}

- (void)setupRunMode
{
    // XXX: populate the lists and set everything up to represent the current options

    // enable all the controls on the Configuration page
    NSEnumerator *enumerator = [[[[tabView tabViewItemAtIndex:0] view] subviews] objectEnumerator];
    NSControl *control;
    while ((control = [enumerator nextObject]))
        [control setEnabled:true];

    [cancelButton setEnabled:true];
    [startButton setEnabled:true];

    [tabView selectTabViewItemAtIndex:0];
}

- (void)setupMessagesMode
{
    [tabView selectTabViewItemAtIndex:1];

    // disable all the controls on the Configuration page except "always show", so the
    // user can enable it if they want to while waiting for something else to happen
    NSEnumerator *enumerator = [[[[tabView tabViewItemAtIndex:0] view] subviews] objectEnumerator];
    NSControl *control;
    while ((control = [enumerator nextObject])) {
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

    if (shouldAutoScroll) {
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
    nsapp = [NSApplication sharedApplication];

    if (startwin != nil) return 1;

    startwin = [[StartupWinController alloc] initWithWindowNibName:@"startwin.editor"];
    if (startwin == nil) return -1;

    [startwin showWindow:nil];
    [startwin setupMessagesMode];

    return 0;
}

int startwin_close(void)
{
    if (startwin == nil) return 1;

    [startwin close];
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
    UNREFERENCED_PARAMETER(v);

    if (startwin) [[startwin window] displayIfNeeded];
    return 0;
}

int startwin_run(void)
{
    int retval;

    if (startwin == nil) return 0;

    [startwin setupRunMode];

    switch ([nsapp runModalForWindow:[startwin window]]) {
        case NSRunStoppedResponse: retval = 1; break;
        case NSRunAbortedResponse: retval = 0; break;
        default: retval = -1;
    }

    [startwin setupMessagesMode];

    return retval;
}
