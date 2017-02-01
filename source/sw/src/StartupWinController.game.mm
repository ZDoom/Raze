//-------------------------------------------------------------------------
/*
 Copyright (C) 2007 Jonathon Fowler <jf@jonof.id.au>

 This file is part of JFShadowWarrior

 Shadow Warrior is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
//-------------------------------------------------------------------------

#import <Cocoa/Cocoa.h>

#include "compat.h"
#include "types.h"
#include "build.h"
#include "baselayer.h"
#include "grpscan.h"
#include "gamedefs.h"
#include "config.h"

#import "GrpFile.game.h"
#import "GameListSource.game.h"

static struct {
	int fullscreen;
	int xdim3d, ydim3d, bpp3d;
	int forcesetup;
	char selectedgrp[BMAX_PATH+1];
    int samplerate, bitspersample, channels;
    int usemouse, usejoystick;
} settings;

static struct soundQuality_t {
    int frequency;
    int samplesize;
    int channels;
} * soundQualities = 0;


@interface StartupWinController : NSWindowController
{
	NSMutableArray *modeslist3d;
	GameListSource *gamelistsrc;

	IBOutlet NSButton *alwaysShowButton;
	IBOutlet NSButton *fullscreenButton;
    IBOutlet NSButton *useMouseButton;
    IBOutlet NSButton *useJoystickButton;
	IBOutlet NSTextView *messagesView;
	IBOutlet NSTabView *tabView;
	IBOutlet NSPopUpButton *videoMode3DPUButton;
    IBOutlet NSPopUpButton *soundQualityPUButton;
	IBOutlet NSScrollView *gameList;

	IBOutlet NSButton *cancelButton;
	IBOutlet NSButton *startButton;
}

- (void)dealloc;
- (void)populateVideoModes:(BOOL)firstTime;
- (void)populateSoundQuality:(BOOL)firstTime;

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
	[gamelistsrc release];
	[modeslist3d release];
	[super dealloc];
}

- (void)populateVideoModes:(BOOL)firstTime
{
	int i, mode3d, fullscreen = ([fullscreenButton state] == NSOnState);
	int idx3d = -1;
	int xdim, ydim, bpp;

	if (firstTime) {
		xdim = settings.xdim3d;
		ydim = settings.ydim3d;
		bpp  = settings.bpp3d;
	} else {
		mode3d = [[modeslist3d objectAtIndex:[videoMode3DPUButton indexOfSelectedItem]] intValue];
		if (mode3d >= 0) {
			xdim = validmode[mode3d].xdim;
			ydim = validmode[mode3d].ydim;
			bpp = validmode[mode3d].bpp;
		}

	}
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

- (void)populateSoundQuality:(BOOL)firstTime
{
    int i, curidx = -1;

    [soundQualityPUButton removeAllItems];

    for (i = 0; soundQualities[i].frequency > 0; i++) {
        const char *ch;
        switch (soundQualities[i].channels) {
            case 1: ch = "Mono"; break;
            case 2: ch = "Stereo"; break;
            default: ch = "?"; break;
        }

        NSString *s = [NSString stringWithFormat:@"%dkHz, %d-bit, %s",
                       soundQualities[i].frequency / 1000,
                       soundQualities[i].samplesize,
                       ch
                       ];
        [soundQualityPUButton addItemWithTitle:s];

        if (firstTime &&
            soundQualities[i].frequency == settings.samplerate &&
            soundQualities[i].samplesize == settings.bitspersample &&
            soundQualities[i].channels == settings.channels) {
            curidx = i;
        }
    }

    if (firstTime && curidx < 0) {
        soundQualities[i].frequency = settings.samplerate;
        soundQualities[i].samplesize = settings.bitspersample;
        soundQualities[i].channels = settings.channels;

        const char *ch;
        switch (soundQualities[i].channels) {
            case 1: ch = "Mono"; break;
            case 2: ch = "Stereo"; break;
            default: ch = "?"; break;
        }
        NSString *s = [NSString stringWithFormat:@"%dkHz, %d-bit, %s",
                       soundQualities[i].frequency / 1000,
                       soundQualities[i].samplesize,
                       ch
                       ];
        [soundQualityPUButton addItemWithTitle:s];

        curidx = i++;
        soundQualities[i].frequency = -1;
    }

    if (curidx >= 0) {
        [soundQualityPUButton selectItemAtIndex:curidx];
    }
}

- (IBAction)alwaysShowClicked:(id)sender
{
}

- (IBAction)fullscreenClicked:(id)sender
{
	[self populateVideoModes:NO];
}

- (IBAction)cancel:(id)sender
{
	[NSApp abortModal];
}

- (IBAction)start:(id)sender
{
	int mode = [[modeslist3d objectAtIndex:[videoMode3DPUButton indexOfSelectedItem]] intValue];
	if (mode >= 0) {
		settings.xdim3d = validmode[mode].xdim;
		settings.ydim3d = validmode[mode].ydim;
		settings.bpp3d = validmode[mode].bpp;
		settings.fullscreen = validmode[mode].fs;
	}

    int quality = [soundQualityPUButton indexOfSelectedItem];
    if (quality >= 0) {
        settings.samplerate = soundQualities[quality].frequency;
        settings.bitspersample = soundQualities[quality].samplesize;
        settings.channels = soundQualities[quality].channels;
    }

	int row = [[gameList documentView] selectedRow];
	if (row >= 0) {
		struct grpfile *p = [[gamelistsrc grpAtIndex:row] entryptr];
		if (p) {
			strcpy(settings.selectedgrp, p->name);
		}
	}

    settings.usemouse = [useMouseButton state] == NSOnState;
    settings.usejoystick = [useJoystickButton state] == NSOnState;
	settings.forcesetup = [alwaysShowButton state] == NSOnState;

	[NSApp stopModal];
}

- (void)setupRunMode
{
	getvalidmodes();

	[fullscreenButton setState: (settings.fullscreen ? NSOnState : NSOffState)];
	[alwaysShowButton setState: (settings.forcesetup ? NSOnState : NSOffState)];
    [useMouseButton setState: (settings.usemouse ? NSOnState : NSOffState)];
    [useJoystickButton setState: (settings.usejoystick ? NSOnState : NSOffState)];
	[self populateVideoModes:YES];
    [self populateSoundQuality:YES];

	// enable all the controls on the Configuration page
	NSEnumerator *enumerator = [[[[tabView tabViewItemAtIndex:0] view] subviews] objectEnumerator];
	NSControl *control;
	while ((control = [enumerator nextObject])) {
        [control setEnabled:true];
    }

	gamelistsrc = [[GameListSource alloc] init];
	[[gameList documentView] setDataSource:gamelistsrc];
	[[gameList documentView] deselectAll:nil];

	int row = [gamelistsrc findIndexForGrpname:[NSString stringWithUTF8String:settings.selectedgrp]];
	if (row >= 0) {
		[[gameList documentView] scrollRowToVisible:row];
		[[gameList documentView] selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];
	}

	[cancelButton setEnabled:true];
	[startButton setEnabled:true];

	[tabView selectTabViewItemAtIndex:0];
	[NSCursor unhide];	// Why should I need to do this?
}

- (void)setupMessagesMode
{
	[tabView selectTabViewItemAtIndex:2];

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
	if (startwin != nil) return 1;

	startwin = [[StartupWinController alloc] initWithWindowNibName:@"startwin.game"];
	if (startwin == nil) return -1;

    {
        static unsigned soundQualityFrequencies[] = { 44100, 22050, 11025 };
        static unsigned soundQualitySampleSizes[] = { 16, 8 };
        static unsigned soundQualityChannels[]    = { 2, 1 };
        unsigned f, b, c, i;

        i = sizeof(soundQualityFrequencies) *
            sizeof(soundQualitySampleSizes) *
            sizeof(soundQualityChannels) /
            sizeof(int) + 2;    // one for the terminator, one for a custom setting
        soundQualities = (struct soundQuality_t *) malloc(i * sizeof(struct soundQuality_t));

        i = 0;
        for (c = 0; c < sizeof(soundQualityChannels) / sizeof(int); c++) {
            for (b = 0; b < sizeof(soundQualitySampleSizes) / sizeof(int); b++) {
                for (f = 0; f < sizeof(soundQualityFrequencies) / sizeof(int); f++) {
                    soundQualities[i].frequency = soundQualityFrequencies[f];
                    soundQualities[i].samplesize = soundQualitySampleSizes[b];
                    soundQualities[i].channels = soundQualityChannels[c];

                    i++;
                }
            }
        }

        soundQualities[i].frequency = -1;
    }

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

	ns = [[NSString alloc] initWithCString:s];
	[startwin putsMessage:ns];
	[ns release];

	return 0;
}

int startwin_settitle(const char *s)
{
	NSString *ns;

	if (!s) return -1;
	if (startwin == nil) return 1;

	ns = [[NSString alloc] initWithCString:s];
	[startwin setTitle:ns];
	[ns release];

	return 0;
}

int startwin_idle(void *v)
{
	if (startwin) [[startwin window] displayIfNeeded];
	return 0;
}

extern char* grpfile;
extern int32 ScreenMode, ScreenWidth, ScreenHeight, ScreenBPP, ForceSetup, UseMouse, UseJoystick;

int startwin_run(void)
{
	int retval;

	if (startwin == nil) return 0;

	ScanGroups();

	settings.fullscreen = ScreenMode;
	settings.xdim3d = ScreenWidth;
	settings.ydim3d = ScreenHeight;
	settings.bpp3d = ScreenBPP;
    settings.samplerate = MixRate;
    settings.bitspersample = NumBits;
    settings.channels = NumChannels;
    settings.usemouse = UseMouse;
    settings.usejoystick = UseJoystick;
	settings.forcesetup = ForceSetup;
	strncpy(settings.selectedgrp, grpfile, BMAX_PATH);

	[startwin setupRunMode];

	switch ([NSApp runModalForWindow:[startwin window]]) {
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

	if (retval) {
		ScreenMode = settings.fullscreen;
		ScreenWidth = settings.xdim3d;
		ScreenHeight = settings.ydim3d;
		ScreenBPP = settings.bpp3d;
        MixRate = settings.samplerate;
        NumBits = settings.bitspersample;
        NumChannels = settings.channels;
        UseMouse = settings.usemouse;
        UseJoystick = settings.usejoystick;
		ForceSetup = settings.forcesetup;
		grpfile = settings.selectedgrp;
	}

	return retval;
}
