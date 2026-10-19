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

#import "macos-views.h"

#include <algorithm>
#include <cmath>

using QaplaMac::Widget;

// ----------------------------------------------------------------------------------------------
// QPTarget
// ----------------------------------------------------------------------------------------------

@implementation QPTarget

- (void)buttonClicked:(id)sender {
    if (self.widget != nullptr) {
        self.widget->clicked = true;
    }
    QaplaMac::scheduleFrame();
}

- (void)checkboxToggled:(id)sender {
    if (self.widget != nullptr) {
        self.widget->changed = true;
        self.widget->number = ([(NSButton*)sender state] == NSControlStateValueOn) ? 1.0 : 0.0;
    }
    QaplaMac::scheduleFrame();
}

- (void)textEntered:(id)sender {
    if (self.widget != nullptr) {
        // Only Enter: a field also sends its action when it loses the focus, and that is no
        // request to act on what was typed.
        NSEvent* event = [NSApp currentEvent];
        if (event.type == NSEventTypeKeyDown) {
            self.widget->enterPressed = true;
        }
        self.widget->text = [[(NSTextField*)sender stringValue] UTF8String];
        self.widget->changed = true;
    }
    QaplaMac::scheduleFrame();
}

- (void)stepperChanged:(id)sender {
    if (self.widget != nullptr) {
        self.widget->number = [(NSStepper*)sender doubleValue];
        self.widget->stepped = true;
    }
    QaplaMac::scheduleFrame();
}

- (void)sliderChanged:(id)sender {
    if (self.widget != nullptr) {
        self.widget->number = [(NSSlider*)sender doubleValue];
        self.widget->changed = true;
    }
    QaplaMac::scheduleFrame();
}

- (void)popUpChosen:(id)sender {
    if (self.widget != nullptr) {
        // Item 0 of a pull-down button is its title, the current choice; the entries follow.
        self.widget->chosenIndex = static_cast<int>([(NSPopUpButton*)sender indexOfSelectedItem]) - 1;
    }
    QaplaMac::scheduleFrame();
}

- (void)controlTextDidChange:(NSNotification*)notification {
    if (self.widget != nullptr) {
        NSTextField* field = notification.object;
        self.widget->text = [field.stringValue UTF8String];
        self.widget->changed = true;
        self.widget->editing = true;
    }
    QaplaMac::scheduleFrame();
}

- (void)controlTextDidBeginEditing:(NSNotification*)notification {
    if (self.widget != nullptr) {
        self.widget->editing = true;
    }
}

- (void)controlTextDidEndEditing:(NSNotification*)notification {
    if (self.widget != nullptr) {
        NSTextField* field = notification.object;
        self.widget->text = [field.stringValue UTF8String];
        self.widget->editing = false;
        self.widget->deactivated = true;
    }
    QaplaMac::scheduleFrame();
}

- (void)textDidChange:(NSNotification*)notification {
    if (self.widget != nullptr) {
        NSTextView* textView = notification.object;
        self.widget->text = [textView.string UTF8String];
        self.widget->changed = true;
    }
    QaplaMac::scheduleFrame();
}

- (void)menuWillOpen:(NSMenu*)menu {
    // A combo's menu is being looked at: the frames leave it alone until it closes.
    if (self.widget != nullptr) {
        self.widget->editing = true;
    }
}

- (void)menuDidClose:(NSMenu*)menu {
    if (self.widget != nullptr) {
        self.widget->editing = false;
    }
    QaplaMac::scheduleFrame();
}

- (BOOL)textView:(NSTextView*)textView doCommandBySelector:(SEL)selector {
    if (self.widget == nullptr || !self.widget->submitOnEnter) {
        return NO;
    }
    if (selector == @selector(insertNewline:)) {
        const NSEventModifierFlags flags = [NSApp currentEvent].modifierFlags;
        if ((flags & (NSEventModifierFlagControl | NSEventModifierFlagCommand)) != 0) {
            [textView insertNewlineIgnoringFieldEditor:self];
        } else {
            self.widget->text = [textView.string UTF8String];
            self.widget->enterPressed = true;
            QaplaMac::scheduleFrame();
        }
        return YES;
    }
    return NO;
}

@end

// ----------------------------------------------------------------------------------------------
// Shared drawing helpers
// ----------------------------------------------------------------------------------------------

namespace {

    NSColor* accent() {
        return [NSColor controlAccentColor];
    }

    void drawTitle(NSString* title, NSRect rect, NSColor* color, NSFont* font, CGFloat x) {
        NSDictionary* attrs = @{ NSFontAttributeName: font, NSForegroundColorAttributeName: color };
        NSSize size = [title sizeWithAttributes:attrs];
        NSPoint point = NSMakePoint(rect.origin.x + x,
            rect.origin.y + std::floor((rect.size.height - size.height) / 2.0));
        [title drawAtPoint:point withAttributes:attrs];
    }

    NSTrackingArea* trackHover(NSView* view, NSTrackingArea* previous) {
        if (previous != nil) {
            [view removeTrackingArea:previous];
        }
        NSTrackingArea* area = [[NSTrackingArea alloc] initWithRect:NSZeroRect
            options:NSTrackingMouseEnteredAndExited | NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect
            owner:view userInfo:nil];
        [view addTrackingArea:area];
        return area;
    }

}

// ----------------------------------------------------------------------------------------------
// QPSectionView
// ----------------------------------------------------------------------------------------------

@implementation QPSectionView {
    NSTrackingArea* tracking_;
    BOOL hovered_;
}

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self != nil) {
        _enabled = YES;
        tracking_ = trackHover(self, nil);
    }
    return self;
}

- (BOOL)isFlipped {
    return YES;
}

- (void)setTitle:(NSString*)title {
    if (![_title isEqualToString:title]) {
        _title = [title copy];
        [self setNeedsDisplay:YES];
    }
}

- (void)setOpen:(BOOL)open {
    if (_open != open) {
        _open = open;
        [self setNeedsDisplay:YES];
    }
}

- (void)setSelected:(BOOL)selected {
    if (_selected != selected) {
        _selected = selected;
        [self setNeedsDisplay:YES];
    }
}

- (void)setLeaf:(BOOL)leaf {
    if (_leaf != leaf) {
        _leaf = leaf;
        [self setNeedsDisplay:YES];
    }
}

- (void)mouseEntered:(NSEvent*)event {
    hovered_ = YES;
    [self setNeedsDisplay:YES];
}

- (void)mouseExited:(NSEvent*)event {
    hovered_ = NO;
    [self setNeedsDisplay:YES];
}

- (void)mouseDown:(NSEvent*)event {
    if (self.enabled && self.widget != nullptr) {
        self.widget->clicked = true;
        QaplaMac::scheduleFrame();
    }
}

- (void)drawRect:(NSRect)dirtyRect {
    NSRect bounds = self.bounds;
    NSColor* background = self.selected
        ? [accent() colorWithAlphaComponent:0.45]
        : [NSColor colorWithWhite:1.0 alpha:(hovered_ && !self.leaf ? 0.13 : 0.08)];
    [background setFill];
    [[NSBezierPath bezierPathWithRoundedRect:bounds xRadius:4 yRadius:4] fill];

    NSColor* textColor = self.enabled ? [NSColor labelColor] : [NSColor tertiaryLabelColor];
    CGFloat textX = 8.0;
    if (!self.leaf) {
        // The disclosure arrow: pointing right when closed, down when open.
        const CGFloat size = 8.0;
        const CGFloat cx = 12.0;
        const CGFloat cy = bounds.size.height / 2.0;
        NSBezierPath* arrow = [NSBezierPath bezierPath];
        if (self.open) {
            [arrow moveToPoint:NSMakePoint(cx - size / 2, cy - size / 4)];
            [arrow lineToPoint:NSMakePoint(cx + size / 2, cy - size / 4)];
            [arrow lineToPoint:NSMakePoint(cx, cy + size / 2.5)];
        } else {
            [arrow moveToPoint:NSMakePoint(cx - size / 4, cy - size / 2)];
            [arrow lineToPoint:NSMakePoint(cx + size / 2.5, cy)];
            [arrow lineToPoint:NSMakePoint(cx - size / 4, cy + size / 2)];
        }
        [arrow closePath];
        [[NSColor secondaryLabelColor] setFill];
        [arrow fill];
        textX = 24.0;
    }
    if (self.title != nil) {
        drawTitle(self.title, bounds, textColor, [NSFont systemFontOfSize:13 weight:NSFontWeightMedium], textX);
    }
}

@end

// ----------------------------------------------------------------------------------------------
// QPSelectableView
// ----------------------------------------------------------------------------------------------

@implementation QPSelectableView {
    NSTrackingArea* tracking_;
    BOOL hovered_;
}

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self != nil) {
        _enabled = YES;
        tracking_ = trackHover(self, nil);
    }
    return self;
}

- (BOOL)isFlipped {
    return YES;
}

- (void)setTitle:(NSString*)title {
    if (![_title isEqualToString:title]) {
        _title = [title copy];
        [self setNeedsDisplay:YES];
    }
}

- (void)setSelected:(BOOL)selected {
    if (_selected != selected) {
        _selected = selected;
        [self setNeedsDisplay:YES];
    }
}

- (void)mouseEntered:(NSEvent*)event {
    hovered_ = YES;
    [self setNeedsDisplay:YES];
}

- (void)mouseExited:(NSEvent*)event {
    hovered_ = NO;
    [self setNeedsDisplay:YES];
}

- (void)mouseDown:(NSEvent*)event {
    if (self.enabled && self.widget != nullptr) {
        self.widget->clicked = true;
        QaplaMac::scheduleFrame();
    }
}

- (void)drawRect:(NSRect)dirtyRect {
    if (self.selected) {
        [[accent() colorWithAlphaComponent:0.45] setFill];
        [[NSBezierPath bezierPathWithRoundedRect:self.bounds xRadius:3 yRadius:3] fill];
    } else if (hovered_ && self.enabled) {
        [[NSColor colorWithWhite:1.0 alpha:0.08] setFill];
        [[NSBezierPath bezierPathWithRoundedRect:self.bounds xRadius:3 yRadius:3] fill];
    }
    if (self.title != nil) {
        NSColor* color = self.enabled ? [NSColor labelColor] : [NSColor tertiaryLabelColor];
        drawTitle(self.title, self.bounds, color, [NSFont systemFontOfSize:13], 4.0);
    }
}

@end

// ----------------------------------------------------------------------------------------------
// QPTabStripView
// ----------------------------------------------------------------------------------------------

namespace {
    struct TabLayout {
        NSRect rect;
        NSRect closeRect;
    };

    constexpr CGFloat TAB_PADDING = 8.0;
    constexpr CGFloat TAB_CLOSE_SIZE = 14.0;
    constexpr CGFloat TAB_GAP = 2.0;
    constexpr CGFloat ADD_BUTTON_WIDTH = 26.0;
}

@implementation QPTabStripView {
    std::vector<QaplaMac::TabInfo> tabs_;
    std::vector<TabLayout> layout_;
    std::uint64_t selected_;
    BOOL addButton_;
    NSRect addRect_;
    CGFloat offset_;
    NSMutableArray<NSString*>* tooltipOwners_;
    NSTrackingArea* tracking_;
    NSPoint mouse_;
    BOOL inside_;
}

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self != nil) {
        tooltipOwners_ = [NSMutableArray array];
        tracking_ = [[NSTrackingArea alloc] initWithRect:NSZeroRect
            options:NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved |
                    NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect
            owner:self userInfo:nil];
        [self addTrackingArea:tracking_];
    }
    return self;
}

- (BOOL)isFlipped {
    return YES;
}

- (NSFont*)tabFont {
    return [NSFont systemFontOfSize:13];
}

- (void)relayout {
    layout_.clear();
    NSDictionary* attrs = @{ NSFontAttributeName: [self tabFont] };
    CGFloat x = -offset_;
    const CGFloat height = self.bounds.size.height;
    for (const auto& tab : tabs_) {
        NSString* title = [NSString stringWithUTF8String:tab.title.c_str()];
        CGFloat width = std::ceil([title sizeWithAttributes:attrs].width) + 2 * TAB_PADDING;
        if (tab.closable) {
            width += TAB_CLOSE_SIZE + 4.0;
        }
        TabLayout entry;
        entry.rect = NSMakeRect(x, 0, width, height);
        entry.closeRect = tab.closable
            ? NSMakeRect(x + width - TAB_PADDING - TAB_CLOSE_SIZE + 4.0, (height - TAB_CLOSE_SIZE) / 2,
                TAB_CLOSE_SIZE, TAB_CLOSE_SIZE)
            : NSZeroRect;
        layout_.push_back(entry);
        x += width + TAB_GAP;
    }
    addRect_ = addButton_ ? NSMakeRect(x, 0, ADD_BUTTON_WIDTH, height) : NSZeroRect;

    [self removeAllToolTips];
    [tooltipOwners_ removeAllObjects];
    for (size_t index = 0; index < tabs_.size(); ++index) {
        if (!tabs_[index].tooltip.empty()) {
            NSString* tip = [NSString stringWithUTF8String:tabs_[index].tooltip.c_str()];
            [tooltipOwners_ addObject:tip];
            [self addToolTipRect:layout_[index].rect owner:tip userData:nullptr];
        }
    }
}

- (CGFloat)contentWidth {
    CGFloat width = 0;
    NSDictionary* attrs = @{ NSFontAttributeName: [self tabFont] };
    for (const auto& tab : tabs_) {
        NSString* title = [NSString stringWithUTF8String:tab.title.c_str()];
        width += std::ceil([title sizeWithAttributes:attrs].width) + 2 * TAB_PADDING + TAB_GAP;
        if (tab.closable) {
            width += TAB_CLOSE_SIZE + 4.0;
        }
    }
    return width + (addButton_ ? ADD_BUTTON_WIDTH : 0);
}

- (void)setTabs:(const std::vector<QaplaMac::TabInfo>&)tabs
       selected:(std::uint64_t)selected
      addButton:(BOOL)addButton {
    tabs_ = tabs;
    selected_ = selected;
    addButton_ = addButton;
    const CGFloat maxOffset = std::max<CGFloat>(0, [self contentWidth] - self.bounds.size.width);
    offset_ = std::min(offset_, maxOffset);
    [self relayout];
    [self setNeedsDisplay:YES];
}

- (void)setFrameSize:(NSSize)newSize {
    [super setFrameSize:newSize];
    [self relayout];
}

- (void)scrollWheel:(NSEvent*)event {
    const CGFloat delta = std::abs(event.scrollingDeltaX) > std::abs(event.scrollingDeltaY)
        ? event.scrollingDeltaX : event.scrollingDeltaY;
    const CGFloat maxOffset = std::max<CGFloat>(0, [self contentWidth] - self.bounds.size.width);
    offset_ = std::clamp<CGFloat>(offset_ - delta, 0, maxOffset);
    [self relayout];
    [self setNeedsDisplay:YES];
}

- (void)mouseMoved:(NSEvent*)event {
    mouse_ = [self convertPoint:event.locationInWindow fromView:nil];
    inside_ = YES;
    [self setNeedsDisplay:YES];
}

- (void)mouseEntered:(NSEvent*)event {
    inside_ = YES;
    [self setNeedsDisplay:YES];
}

- (void)mouseExited:(NSEvent*)event {
    inside_ = NO;
    [self setNeedsDisplay:YES];
}

- (void)mouseDown:(NSEvent*)event {
    if (self.widget == nullptr) {
        return;
    }
    NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    for (size_t index = 0; index < layout_.size(); ++index) {
        if (NSPointInRect(point, layout_[index].closeRect)) {
            self.widget->closedTab = tabs_[index].key;
            QaplaMac::scheduleFrame();
            return;
        }
        if (NSPointInRect(point, layout_[index].rect)) {
            self.widget->clickedTab = tabs_[index].key;
            QaplaMac::scheduleFrame();
            return;
        }
    }
    if (addButton_ && NSPointInRect(point, addRect_)) {
        self.widget->addClicked = true;
        QaplaMac::scheduleFrame();
    }
}

- (void)drawRect:(NSRect)dirtyRect {
    const NSRect bounds = self.bounds;
    // The line the tabs stand on.
    [[NSColor separatorColor] setFill];
    NSRectFill(NSMakeRect(0, bounds.size.height - 1, bounds.size.width, 1));

    NSFont* font = [self tabFont];
    for (size_t index = 0; index < tabs_.size(); ++index) {
        const auto& tab = tabs_[index];
        const auto& entry = layout_[index];
        const bool isSelected = tab.key == selected_;
        const bool isHovered = inside_ && NSPointInRect(mouse_, entry.rect);
        NSRect tabRect = NSInsetRect(entry.rect, 0, 2);
        tabRect.size.height += 2;
        NSColor* fill = isSelected ? [NSColor colorWithWhite:1.0 alpha:0.16]
            : [NSColor colorWithWhite:1.0 alpha:(isHovered ? 0.09 : 0.04)];
        [fill setFill];
        [[NSBezierPath bezierPathWithRoundedRect:tabRect xRadius:5 yRadius:5] fill];
        if (isSelected) {
            [accent() setFill];
            NSRectFill(NSMakeRect(entry.rect.origin.x + 4, bounds.size.height - 2,
                entry.rect.size.width - 8, 2));
        }
        NSString* title = [NSString stringWithUTF8String:tab.title.c_str()];
        NSColor* textColor = isSelected ? [NSColor labelColor] : [NSColor secondaryLabelColor];
        drawTitle(title, entry.rect, textColor, font, TAB_PADDING);
        if (tab.closable && (isSelected || isHovered)) {
            const NSRect close = entry.closeRect;
            const bool overClose = inside_ && NSPointInRect(mouse_, close);
            if (overClose) {
                [[NSColor colorWithWhite:1.0 alpha:0.15] setFill];
                [[NSBezierPath bezierPathWithOvalInRect:close] fill];
            }
            NSBezierPath* cross = [NSBezierPath bezierPath];
            const CGFloat inset = 4.0;
            [cross moveToPoint:NSMakePoint(NSMinX(close) + inset, NSMinY(close) + inset)];
            [cross lineToPoint:NSMakePoint(NSMaxX(close) - inset, NSMaxY(close) - inset)];
            [cross moveToPoint:NSMakePoint(NSMaxX(close) - inset, NSMinY(close) + inset)];
            [cross lineToPoint:NSMakePoint(NSMinX(close) + inset, NSMaxY(close) - inset)];
            cross.lineWidth = 1.2;
            [[NSColor secondaryLabelColor] setStroke];
            [cross stroke];
        }
        if (tab.highlighted) {
            // Something in the tab asks for attention -- the same red dot as everywhere else.
            [[NSColor colorWithSRGBRed:0.75 green:0.0 blue:0.0 alpha:0.85] setFill];
            [[NSBezierPath bezierPathWithOvalInRect:
                NSMakeRect(NSMaxX(entry.rect) - 9, 4, 7, 7)] fill];
        }
    }
    if (addButton_) {
        const bool isHovered = inside_ && NSPointInRect(mouse_, addRect_);
        if (isHovered) {
            [[NSColor colorWithWhite:1.0 alpha:0.09] setFill];
            [[NSBezierPath bezierPathWithRoundedRect:NSInsetRect(addRect_, 1, 3) xRadius:5 yRadius:5] fill];
        }
        NSBezierPath* plus = [NSBezierPath bezierPath];
        const CGFloat cx = NSMidX(addRect_);
        const CGFloat cy = NSMidY(addRect_);
        [plus moveToPoint:NSMakePoint(cx - 5, cy)];
        [plus lineToPoint:NSMakePoint(cx + 5, cy)];
        [plus moveToPoint:NSMakePoint(cx, cy - 5)];
        [plus lineToPoint:NSMakePoint(cx, cy + 5)];
        plus.lineWidth = 1.5;
        [[NSColor secondaryLabelColor] setStroke];
        [plus stroke];
    }
}

@end

// ----------------------------------------------------------------------------------------------
// QPSplitterView
// ----------------------------------------------------------------------------------------------

@implementation QPSplitterView {
    NSPoint last_;
}

- (BOOL)isFlipped {
    return YES;
}

- (void)resetCursorRects {
    if (self.interactive) {
        [self addCursorRect:self.bounds
                     cursor:self.vertical ? [NSCursor resizeUpDownCursor] : [NSCursor resizeLeftRightCursor]];
    }
}

- (void)mouseDown:(NSEvent*)event {
    last_ = event.locationInWindow;
    if (self.widget != nullptr) {
        self.widget->dragging = true;
    }
}

- (void)mouseDragged:(NSEvent*)event {
    if (!self.interactive || self.widget == nullptr) {
        return;
    }
    NSPoint now = event.locationInWindow;
    // Window coordinates grow upwards, QaplaUi's downwards.
    const CGFloat delta = self.vertical ? (last_.y - now.y) : (now.x - last_.x);
    last_ = now;
    self.widget->dragDelta += static_cast<float>(delta);
    QaplaMac::scheduleFrame();
}

- (void)mouseUp:(NSEvent*)event {
    if (self.widget != nullptr) {
        self.widget->dragging = false;
    }
}

- (void)drawRect:(NSRect)dirtyRect {
    // A hairline in the middle of the handle, as macOS draws the divider of a split view; the
    // handle itself is wider so it can be found with the mouse.
    [[NSColor separatorColor] setFill];
    const NSRect bounds = self.bounds;
    if (self.vertical) {
        NSRectFill(NSMakeRect(0, std::floor(bounds.size.height / 2.0), bounds.size.width, 1));
    } else {
        NSRectFill(NSMakeRect(std::floor(bounds.size.width / 2.0), 0, 1, bounds.size.height));
    }
}

@end

// ----------------------------------------------------------------------------------------------
// QPScrollView, QPRootView
// ----------------------------------------------------------------------------------------------

@implementation QPScrollView

- (void)scrollWheel:(NSEvent*)event {
    if (self.ignoresScrollWheel) {
        [self.nextResponder scrollWheel:event];
        return;
    }
    [super scrollWheel:event];
}

@end

@implementation QPRootView

- (BOOL)isFlipped {
    return YES;
}

- (BOOL)isOpaque {
    return YES;
}

- (void)drawRect:(NSRect)dirtyRect {
    // Painted here rather than left to the window, so a picture of the view has it too.
    [[NSColor windowBackgroundColor] setFill];
    NSRectFill(dirtyRect);
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

// The application reads Command+C and Command+V itself (see ImGuiCutPaste). These keep the Edit
// menu's items from beeping when no text field has the focus to take them.
- (void)copy:(id)sender {
}

- (void)paste:(id)sender {
}

- (void)keyDown:(NSEvent*)event {
    // Keys are read by the frame from the event monitor; nothing here may beep at them.
}

- (void)mouseDown:(NSEvent*)event {
    // Clicking beside a text field takes the focus from it, as everywhere else on the Mac.
    [self.window makeFirstResponder:self];
}

@end
