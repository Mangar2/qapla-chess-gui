/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2025 Volker Böhm
 */

/**
 * @file macos-main.mm
 * @brief The native macOS frontend: the application, its window and menus, and the frame loop.
 *
 * The frames are driven by a timer on the main run loop, in the common modes so they go on
 * while a menu is open or the window is being resized: quickly while the user works, more slowly
 * while nothing happens -- the tool queue of the AI chat and the remote control is served in the
 * frames, so they never stop altogether.
 */

#import "macos-views.h"
#import <QuartzCore/QuartzCore.h>

#include "app/app-runtime.h"
#include "command-line.h"
#include "data/logo-data.h"
#include "os-dialogs.h"
#include "os-helpers.h"
#include "ui-thread-watch.h"
#include "version.h"

#include <csignal>
#include <map>
#include <sstream>
#include <iostream>
#include <memory>

namespace {

    std::unique_ptr<QaplaApp::AppRuntime> runtime;
    NSWindow* mainWindow = nil;
    NSTimer* frameTimer = nil;
    bool stopping = false;
    double lastFrame = 0.0;

    constexpr double FAST_INTERVAL = 1.0 / 60.0;
    constexpr double IDLE_INTERVAL = 1.0 / 20.0;

    /** @brief Writes what the window shows to a PNG file. */
    void writeScreenshot(const std::string& target) {
        [mainWindow displayIfNeeded];
        NSView* view = mainWindow.contentView;
        NSBitmapImageRep* rep = [view bitmapImageRepForCachingDisplayInRect:view.bounds];
        [view cacheDisplayInRect:view.bounds toBitmapImageRep:rep];
        // Sheets are windows of their own; a picture of the main window alone would miss them.
        for (NSWindow* sheet in mainWindow.sheets) {
            NSView* sheetView = sheet.contentView;
            NSBitmapImageRep* sheetRep = [sheetView bitmapImageRepForCachingDisplayInRect:sheetView.bounds];
            [sheetView cacheDisplayInRect:sheetView.bounds toBitmapImageRep:sheetRep];
            NSData* sheetPng = [sheetRep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
            std::string sheetTarget = target.substr(0, target.rfind('.')) + "-sheet.png";
            [sheetPng writeToFile:[NSString stringWithUTF8String:sheetTarget.c_str()] atomically:YES];
        }
        NSData* png = [rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
        [png writeToFile:[NSString stringWithUTF8String:target.c_str()] atomically:YES];
        std::cout << "QAPLA_SCREENSHOT " << target << "\n" << std::flush;
    }

    /**
     * @brief Writes what the window shows to a PNG file, if QAPLA_SCREENSHOT asks for it.
     *
     * For a developer checking what the frontend draws where nothing may record the screen.
     * QAPLA_SCREENSHOT names the file, QAPLA_SCREENSHOT_FRAME the frame (default 60) and
     * QAPLA_SCREENSHOT_QUIT=1 ends the application afterwards. QAPLA_SCREENSHOT_INTERVAL=<s>
     * instead writes a numbered picture every so many seconds, for following a session that a
     * script drives over the remote control.
     */
    void screenshotIfRequested(int frame) {
        static const auto path = QaplaHelpers::OsHelpers::getEnv("QAPLA_SCREENSHOT");
        if (!path) {
            return;
        }
        static const auto interval = QaplaHelpers::OsHelpers::getEnv("QAPLA_SCREENSHOT_INTERVAL");
        static const int wantedFrame = [] {
            auto value = QaplaHelpers::OsHelpers::getEnv("QAPLA_SCREENSHOT_FRAME");
            return value ? std::stoi(*value) : 60;
        }();
        static double nextShot = 0.0;
        static int counter = 0;

        std::string target = *path;
        const double now = CACurrentMediaTime();
        if (interval) {
            if (now < nextShot) {
                return;
            }
            nextShot = now + std::stod(*interval);
            auto dot = target.rfind('.');
            target = target.substr(0, dot) + "-" + std::to_string(counter++) +
                (dot == std::string::npos ? std::string(".png") : target.substr(dot));
        } else if (frame != wantedFrame) {
            return;
        }

        writeScreenshot(target);
        if (!interval && QaplaHelpers::OsHelpers::getEnv("QAPLA_SCREENSHOT_QUIT")) {
            runtime->requestQuit();
        }
    }

    // ------------------------------------------------------------------------------------------
    // QAPLA_UI_SCRIPT
    // ------------------------------------------------------------------------------------------

    /**
     * @brief Plays a script of clicks, keys and pictures against the window, for a developer.
     *
     * One command per line, coordinates in points from the top left of the window's content:
     *   wait <seconds>        click <x> <y>        rightclick <x> <y>      move <x> <y>
     *   sclick <x> <y>        (a click into the dialog shown as a sheet)
     *   type <text>           key <enter|tab|escape|up|down|left|right|backspace>
     *   shot <file.png>       quit                 # a comment
     * The events go through AppKit like the user's own, so what a script clicks is what a mouse
     * would have clicked.
     */
    class UiScript {
    public:
        static void startIfRequested() {
            auto path = QaplaHelpers::OsHelpers::getEnv("QAPLA_UI_SCRIPT");
            if (!path) {
                return;
            }
            NSString* text = [NSString stringWithContentsOfFile:[NSString stringWithUTF8String:path->c_str()]
                                                       encoding:NSUTF8StringEncoding error:nil];
            if (text == nil) {
                std::cerr << "QAPLA_UI_SCRIPT: cannot read " << *path << "\n";
                return;
            }
            for (NSString* line in [text componentsSeparatedByString:@"\n"]) {
                NSString* trimmed = [line stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
                if (trimmed.length > 0 && ![trimmed hasPrefix:@"#"]) {
                    lines().push_back(trimmed.UTF8String);
                }
            }
            // A script clicks into a window that has to be the active one, as the user's
            // clicks are; started from a terminal the application may not be.
            [NSApp activateIgnoringOtherApps:YES];
            [mainWindow makeKeyAndOrderFront:nil];
            next(1.0);
        }

    private:
        static std::vector<std::string>& lines() {
            static std::vector<std::string> list;
            return list;
        }

        static std::size_t& position() {
            static std::size_t index = 0;
            return index;
        }

        static void next(double delay) {
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(delay * NSEC_PER_SEC)),
                dispatch_get_main_queue(), ^{ step(); });
        }

        static NSPoint toWindow(NSWindow* window, double x, double y) {
            const NSRect content = window.contentView.bounds;
            return NSMakePoint(x, content.size.height - y);
        }

        static void postMouse(NSWindow* window, NSEventType type, NSPoint location, NSInteger clicks) {
            NSEvent* event = [NSEvent mouseEventWithType:type location:location
                modifierFlags:0 timestamp:[NSProcessInfo processInfo].systemUptime
                windowNumber:window.windowNumber context:nil eventNumber:0
                clickCount:clicks pressure:1.0];
            [NSApp postEvent:event atStart:NO];
        }

        static void postKey(NSString* characters, unsigned short code) {
            for (NSEventType type : { NSEventTypeKeyDown, NSEventTypeKeyUp }) {
                NSEvent* event = [NSEvent keyEventWithType:type location:NSZeroPoint modifierFlags:0
                    timestamp:[NSProcessInfo processInfo].systemUptime
                    windowNumber:mainWindow.windowNumber context:nil characters:characters
                    charactersIgnoringModifiers:characters isARepeat:NO keyCode:code];
                [NSApp postEvent:event atStart:NO];
            }
        }

        static void step() {
            if (stopping || mainWindow == nil) {
                return;
            }
            auto& all = lines();
            auto& index = position();
            if (index >= all.size()) {
                return;
            }
            const std::string line = all[index++];
            std::cout << "QAPLA_UI_SCRIPT " << line << "\n" << std::flush;
            std::istringstream input(line);
            std::string command;
            input >> command;
            double delay = 0.15;
            if (command == "wait") {
                input >> delay;
            } else if (command == "click" || command == "rightclick" || command == "move" ||
                       command == "sclick") {
                double x = 0;
                double y = 0;
                input >> x >> y;
                // sclick: into the dialog shown as a sheet of the window.
                NSWindow* window = command == "sclick" && mainWindow.sheets.count > 0
                    ? mainWindow.sheets.firstObject : mainWindow;
                // A click into a window that is not the key one only makes it key, as with a
                // mouse; the script means the click itself.
                [window makeKeyWindow];
                const NSPoint location = toWindow(window, x, y);
                if (QaplaHelpers::OsHelpers::getEnv("QAPLA_UI_SCRIPT_DEBUG")) {
                    NSView* hit = [window.contentView.superview hitTest:location];
                    std::cout << "  window " << (window == mainWindow ? "main" : "sheet")
                              << " at " << location.x << "," << location.y << " hits "
                              << (hit != nil ? NSStringFromClass([hit class]).UTF8String : "nothing")
                              << "\n";
                }
                postMouse(window, NSEventTypeMouseMoved, location, 0);
                if (command == "click" || command == "sclick") {
                    postMouse(window, NSEventTypeLeftMouseDown, location, 1);
                    postMouse(window, NSEventTypeLeftMouseUp, location, 1);
                } else if (command == "rightclick") {
                    postMouse(window, NSEventTypeRightMouseDown, location, 1);
                    postMouse(window, NSEventTypeRightMouseUp, location, 1);
                }
            } else if (command == "type") {
                std::string rest;
                std::getline(input, rest);
                if (!rest.empty() && rest.front() == ' ') {
                    rest.erase(0, 1);
                }
                for (char character : rest) {
                    postKey([NSString stringWithFormat:@"%c", character], 0);
                }
            } else if (command == "key") {
                std::string name;
                input >> name;
                static const std::map<std::string, std::pair<unichar, unsigned short>> keys = {
                    { "enter", { '\r', 36 } }, { "tab", { '\t', 48 } }, { "escape", { 27, 53 } },
                    { "backspace", { 127, 51 } }, { "up", { NSUpArrowFunctionKey, 126 } },
                    { "down", { NSDownArrowFunctionKey, 125 } }, { "left", { NSLeftArrowFunctionKey, 123 } },
                    { "right", { NSRightArrowFunctionKey, 124 } },
                };
                auto found = keys.find(name);
                if (found != keys.end()) {
                    const unichar character = found->second.first;
                    postKey([NSString stringWithCharacters:&character length:1], found->second.second);
                }
            } else if (command == "shot") {
                std::string file;
                input >> file;
                writeScreenshot(file);
            } else if (command == "quit") {
                runtime->requestQuit();
                return;
            }
            next(delay);
        }
    };

    void stopApplication() {
        if (stopping) {
            return;
        }
        stopping = true;
        [frameTimer invalidate];
        frameTimer = nil;
        [NSApp stop:nil];
        // stop: takes effect once an event has been handled; this is that event.
        NSEvent* wake = [NSEvent otherEventWithType:NSEventTypeApplicationDefined location:NSZeroPoint
            modifierFlags:0 timestamp:0 windowNumber:0 context:nil subtype:0 data1:0 data2:0];
        [NSApp postEvent:wake atStart:YES];
    }

    void runFrame() {
        static bool inFrame = false;
        static int frameCount = 0;
        if (inFrame || stopping || runtime == nullptr || mainWindow == nil) {
            return;
        }
        inFrame = true;
        lastFrame = CACurrentMediaTime();

        QaplaMac::beginFrame(mainWindow);
        runtime->beginFrame();
        {
            QaplaWindows::UiThreadWatch::Section section("draw");
            runtime->draw();
        }
        {
            QaplaWindows::UiThreadWatch::Section section("render");
            QaplaMac::endFrame();
        }
        screenshotIfRequested(++frameCount);
        runtime->endFrame();
        inFrame = false;

        if (runtime->quitRequested()) {
            stopApplication();
        }
    }

    void tick() {
        const double interval = QaplaMac::wantsFastFrames() ? FAST_INTERVAL : IDLE_INTERVAL;
        if (CACurrentMediaTime() - lastFrame + 0.002 < interval) {
            return;
        }
        runFrame();
    }

    NSMenuItem* addItem(NSMenu* menu, NSString* title, SEL action, NSString* key,
        NSEventModifierFlags modifiers = NSEventModifierFlagCommand) {
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title action:action keyEquivalent:key];
        item.keyEquivalentModifierMask = modifiers;
        [menu addItem:item];
        return item;
    }

    void buildMenus() {
        NSMenu* bar = [[NSMenu alloc] init];
        NSString* appName = @"Qapla Chess";

        NSMenuItem* appItem = [[NSMenuItem alloc] init];
        NSMenu* appMenu = [[NSMenu alloc] initWithTitle:appName];
        addItem(appMenu, [@"About " stringByAppendingString:appName],
            @selector(orderFrontStandardAboutPanel:), @"", 0);
        [appMenu addItem:[NSMenuItem separatorItem]];
        NSMenuItem* services = addItem(appMenu, @"Services", nil, @"", 0);
        NSMenu* servicesMenu = [[NSMenu alloc] initWithTitle:@"Services"];
        services.submenu = servicesMenu;
        [NSApp setServicesMenu:servicesMenu];
        [appMenu addItem:[NSMenuItem separatorItem]];
        addItem(appMenu, [@"Hide " stringByAppendingString:appName], @selector(hide:), @"h");
        addItem(appMenu, @"Hide Others", @selector(hideOtherApplications:), @"h",
            NSEventModifierFlagCommand | NSEventModifierFlagOption);
        addItem(appMenu, @"Show All", @selector(unhideAllApplications:), @"", 0);
        [appMenu addItem:[NSMenuItem separatorItem]];
        addItem(appMenu, [@"Quit " stringByAppendingString:appName], @selector(terminate:), @"q");
        appItem.submenu = appMenu;
        [bar addItem:appItem];

        NSMenuItem* editItem = [[NSMenuItem alloc] init];
        NSMenu* editMenu = [[NSMenu alloc] initWithTitle:@"Edit"];
        addItem(editMenu, @"Undo", @selector(undo:), @"z");
        addItem(editMenu, @"Redo", @selector(redo:), @"z",
            NSEventModifierFlagCommand | NSEventModifierFlagShift);
        [editMenu addItem:[NSMenuItem separatorItem]];
        addItem(editMenu, @"Cut", @selector(cut:), @"x");
        addItem(editMenu, @"Copy", @selector(copy:), @"c");
        addItem(editMenu, @"Paste", @selector(paste:), @"v");
        addItem(editMenu, @"Select All", @selector(selectAll:), @"a");
        editItem.submenu = editMenu;
        [bar addItem:editItem];

        NSMenuItem* viewItem = [[NSMenuItem alloc] init];
        NSMenu* viewMenu = [[NSMenu alloc] initWithTitle:@"View"];
        addItem(viewMenu, @"Enter Full Screen", @selector(toggleFullScreen:), @"f",
            NSEventModifierFlagCommand | NSEventModifierFlagControl);
        viewItem.submenu = viewMenu;
        [bar addItem:viewItem];

        NSMenuItem* windowItem = [[NSMenuItem alloc] init];
        NSMenu* windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
        addItem(windowMenu, @"Minimize", @selector(performMiniaturize:), @"m");
        addItem(windowMenu, @"Zoom", @selector(performZoom:), @"", 0);
        windowItem.submenu = windowMenu;
        [bar addItem:windowItem];
        [NSApp setWindowsMenu:windowMenu];

        [NSApp setMainMenu:bar];
    }

} // namespace

@interface QPAppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@end

@implementation QPAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    [NSApp activateIgnoringOtherApps:YES];
    [mainWindow makeKeyAndOrderFront:nil];
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender {
    // Command+Q ends the application the way its close button does: through the frame loop,
    // which runs the shutdown that saves the session.
    if (runtime != nullptr) {
        runtime->requestQuit();
    }
    return NSTerminateCancel;
}

- (BOOL)windowShouldClose:(NSWindow*)sender {
    if (runtime != nullptr) {
        runtime->requestQuit();
    }
    return NO;
}

- (void)frameTimerFired:(NSTimer*)timer {
    tick();
}

@end

int main(int argc, char** argv) {
    // Ignore SIGPIPE to prevent crashes when writing to closed pipes (e.g., chess engines)
    std::signal(SIGPIPE, SIG_IGN);

    @autoreleasepool {
        try {
            auto options = QaplaApp::parseCommandLine(argc, argv);
            QaplaApp::reportCommandLineMessages(options);
            if (options.helpRequested) {
                std::cout << QaplaApp::helpText() << std::flush;
                return 0;
            }
            if (!QaplaApp::applyConfigDirectory(options)) {
                return 1;
            }
            QaplaApp::applySessionOptions(options);

            [NSApplication sharedApplication];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
            // The application was designed dark -- the board's coordinates, the clocks and the
            // icons are drawn for a dark background -- and keeps that look here.
            NSApp.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
            QPAppDelegate* delegate = [[QPAppDelegate alloc] init];
            NSApp.delegate = delegate;
            buildMenus();

            NSData* logo = [NSData dataWithBytes:logopng length:logopngSize];
            NSImage* icon = [[NSImage alloc] initWithData:logo];
            if (icon != nil) {
                NSApp.applicationIconImage = icon;
            }

            QaplaMac::registerFonts();
            runtime = std::make_unique<QaplaApp::AppRuntime>(options.remoteControl, false);

            const std::string title = "Qapla Chess GUI " + std::string(QaplaApp::VERSION);
            mainWindow = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1400, 800)
                styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                          NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
                backing:NSBackingStoreBuffered defer:NO];
            mainWindow.title = [NSString stringWithUTF8String:title.c_str()];
            mainWindow.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
            mainWindow.releasedWhenClosed = NO;
            mainWindow.delegate = delegate;
            mainWindow.acceptsMouseMovedEvents = YES;
            mainWindow.minSize = NSMakeSize(800, 500);
            mainWindow.contentView = [[QPRootView alloc] initWithFrame:NSMakeRect(0, 0, 1400, 800)];
            mainWindow.tabbingMode = NSWindowTabbingModeDisallowed;
            [mainWindow center];
            [mainWindow setFrameAutosaveName:@"QaplaMainWindow"];
            [mainWindow makeKeyAndOrderFront:nil];
            [mainWindow makeFirstResponder:mainWindow.contentView];

            QaplaWindows::OsDialogs::setNativeWindowProvider(
                []() -> void* { return (__bridge void*)mainWindow; });

            [NSEvent addLocalMonitorForEventsMatchingMask:
                NSEventMaskLeftMouseDown | NSEventMaskLeftMouseUp | NSEventMaskRightMouseDown |
                NSEventMaskRightMouseUp | NSEventMaskOtherMouseDown | NSEventMaskOtherMouseUp |
                NSEventMaskKeyDown | NSEventMaskKeyUp | NSEventMaskMouseMoved |
                NSEventMaskLeftMouseDragged | NSEventMaskScrollWheel
                handler:^NSEvent*(NSEvent* event) {
                    QaplaMac::recordEvent(event);
                    return event;
                }];

            QaplaMac::setFrameScheduler(&runFrame);
            frameTimer = [NSTimer timerWithTimeInterval:FAST_INTERVAL target:delegate
                selector:@selector(frameTimerFired:) userInfo:nil repeats:YES];
            frameTimer.tolerance = 0.002;
            [[NSRunLoop mainRunLoop] addTimer:frameTimer forMode:NSRunLoopCommonModes];

            UiScript::startIfRequested();
            [NSApp run];

            runtime->shutdownBeforeWindowCloses();
            [mainWindow orderOut:nil];
            mainWindow = nil;
            runtime->shutdownAfterWindowClosed();
            runtime.reset();
            return 0;
        }
        catch (const std::exception& e) {
            std::cerr << "Fatal error: " << e.what() << '\n';
            return 1;
        }
    }
}
