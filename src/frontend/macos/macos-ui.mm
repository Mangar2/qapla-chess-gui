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
 * @file macos-ui.mm
 * @brief QaplaUi on AppKit: containers, layout, input and the controls.
 *
 * See macos-backend.h for how a frame's description becomes views that last.
 */

#import "macos-views.h"
#import <QuartzCore/QuartzCore.h>

#include "imgui-table.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>
#include <numbers>
#include <set>

using QaplaUi::Vec2;
using QaplaUi::Vec4;
using QaplaUi::Color32;
using QaplaUi::StyleColor;
using QaplaUi::StyleVar;

namespace QaplaMac {

    namespace {

        // --------------------------------------------------------------------------------------
        // State
        // --------------------------------------------------------------------------------------

        struct InputEvent {
            enum class Type : std::uint8_t { MouseDown, MouseUp, MouseMove, KeyDown, KeyUp } type;
            int button = 0;
            NSWindow* window = nil;
            NSPoint location = NSZeroPoint;   ///< In the window's base coordinates
            unsigned short keyCode = 0;
            bool repeat = false;
        };

        struct Pressed {
            QaplaUi::Key key;
            bool repeat;
        };

        struct PopupState {
            bool open = false;
            Vec2 position;
            int openedFrame = -1;
            bool menuShown = false;
            std::optional<std::size_t> menuChoice;
        };

        struct PopupScope {
            std::uint64_t key;
            bool modal;
        };

        constexpr std::size_t STYLE_COLOR_COUNT = 15;

        struct State {
            NSWindow* window = nil;
            QPRootView* root = nil;
            int frame = 0;
            bool inFrame = false;
            double startTime = 0.0;
            double frameTime = 0.0;
            float deltaTime = 1.0F / 60.0F;
            double fastUntil = 0.0;
            void (*scheduler)() = nullptr;
            bool frameScheduled = false;

            std::unique_ptr<Container> implicitRoot;
            std::unordered_map<std::uint64_t, std::unique_ptr<Container>> overlays;
            std::unordered_map<std::uint64_t, std::unique_ptr<Container>> popupContainers;
            std::unordered_map<std::uint64_t, std::unique_ptr<Container>> modals;
            std::vector<Container*> overlayOrder;
            std::vector<Container*> popupOrder;
            std::vector<Container*> stack;
            std::vector<std::uint64_t> idStack;

            std::vector<QaplaUi::Font> fonts;
            std::array<std::vector<Color32>, STYLE_COLOR_COUNT> colorStacks;
            std::vector<StyleColor> colorOrder;   ///< Which stack each push went to, for the pops
            std::vector<std::pair<StyleVar, Vec2>> varStack;
            QaplaUi::Style style;
            int disabled = 0;
            std::vector<bool> disabledStack;
            std::vector<float> wrapStack;

            Widget* combo = nullptr;
            std::vector<Widget*> tabBars;
            std::unordered_map<std::uint64_t, PopupState> popups;
            std::vector<PopupScope> popupScopes;

            std::deque<InputEvent> events;
            std::unordered_map<void*, Vec2> mouseByWindow;   ///< Last known position, per window
            Vec2 mouse;
            Vec2 previousMouse;
            std::array<bool, 3> down{};
            std::array<bool, 3> clicked{};
            std::array<bool, 3> released{};
            std::array<NSWindow*, 3> eventWindow{};
            std::vector<Pressed> pressed;
            std::set<int> keysDown;
            NSEventModifierFlags modifiers = 0;
            std::unordered_map<void*, NSView*> hitViews;
            std::uint64_t activeKey = 0;
        };

        State& state() {
            static State instance;
            return instance;
        }

        Container& current() {
            return *state().stack.back();
        }

        // --------------------------------------------------------------------------------------
        // Identifiers and labels
        // --------------------------------------------------------------------------------------

        constexpr std::uint64_t FNV_OFFSET = 0xcbf29ce484222325ULL;
        constexpr std::uint64_t FNV_PRIME = 0x100000001b3ULL;

        std::uint64_t hashBytes(std::uint64_t seed, const void* data, std::size_t size) {
            std::uint64_t hash = seed;
            const auto* bytes = static_cast<const unsigned char*>(data);
            for (std::size_t index = 0; index < size; ++index) {
                hash ^= bytes[index];
                hash *= FNV_PRIME;
            }
            return hash;
        }

        std::uint64_t seed() {
            return state().idStack.empty() ? FNV_OFFSET : state().idStack.back();
        }

        /** @brief The part of a label that identifies it: after "###" if there is one. */
        std::string_view idPart(std::string_view label) {
            auto pos = label.find("###");
            return pos == std::string_view::npos ? label : label.substr(pos + 3);
        }

        /** @brief The part of a label that is shown: before "##". */
        std::string_view displayPart(std::string_view label) {
            auto pos = label.find("##");
            return pos == std::string_view::npos ? label : label.substr(0, pos);
        }

        std::uint64_t keyFor(std::string_view label) {
            auto part = idPart(label);
            return hashBytes(seed(), part.data(), part.size());
        }

        /** @brief A key unique within its container for this frame, even if a label repeats. */
        std::uint64_t uniqueKey(Container& container, std::uint64_t key) {
            auto& uses = container.keyUses[key];
            ++uses;
            if (uses == 1) {
                return key;
            }
            return hashBytes(key, &uses, sizeof(uses));
        }

        NSString* toNS(std::string_view text) {
            NSString* result = [[NSString alloc] initWithBytes:text.data() length:text.size()
                                                      encoding:NSUTF8StringEncoding];
            return result != nil ? result : @"";
        }

        std::string fromNS(NSString* text) {
            const char* utf8 = text.UTF8String;
            return utf8 != nullptr ? std::string(utf8) : std::string();
        }

        // --------------------------------------------------------------------------------------
        // Style
        // --------------------------------------------------------------------------------------

        Color32 defaultColor(StyleColor which) {
            using QaplaUi::col32;
            switch (which) {
            case StyleColor::Text: return col32(235, 235, 235);
            case StyleColor::TextDisabled: return col32(128, 128, 128);
            case StyleColor::WindowBg: return col32(30, 30, 30);
            case StyleColor::PopupBg: return col32(44, 44, 46, 250);
            case StyleColor::Border: return col32(255, 255, 255, 30);
            case StyleColor::BorderShadow: return col32(0, 0, 0, 90);
            case StyleColor::FrameBg: return col32(255, 255, 255, 16);
            case StyleColor::Button: return col32(255, 255, 255, 26);
            case StyleColor::ButtonHovered: return col32(255, 255, 255, 46);
            case StyleColor::ButtonActive: return col32(10, 132, 255, 160);
            case StyleColor::Header: return col32(255, 255, 255, 22);
            case StyleColor::HeaderHovered: return col32(255, 255, 255, 36);
            case StyleColor::Separator: return col32(255, 255, 255, 30);
            case StyleColor::TableRowBg: return col32(0, 0, 0, 0);
            case StyleColor::TabDimmedSelected: return col32(10, 132, 255, 90);
            }
            return col32(235, 235, 235);
        }

        Color32 styleColor(StyleColor which) {
            const auto& stack = state().colorStacks[static_cast<std::size_t>(which)];
            return stack.empty() ? defaultColor(which) : stack.back();
        }

        Color32 scaleAlpha(Color32 color, float factor) {
            const auto alpha = static_cast<std::uint32_t>(
                std::clamp(static_cast<float>(color >> 24U) * factor, 0.0F, 255.0F));
            return (color & 0x00FFFFFFU) | (alpha << 24U);
        }

        constexpr float DISABLED_ALPHA = 0.6F;

        QaplaUi::Style baseStyle() {
            QaplaUi::Style style;
            style.windowPadding = { 8.0F, 8.0F };
            style.framePadding = { 4.0F, 3.0F };
            style.itemSpacing = { 8.0F, 4.0F };
            style.frameRounding = 4.0F;
            style.scrollbarSize = 14.0F;
            return style;
        }

        void applyVars() {
            auto& s = state();
            s.style = baseStyle();
            for (const auto& [var, value] : s.varStack) {
                switch (var) {
                case StyleVar::WindowPadding: s.style.windowPadding = value; break;
                case StyleVar::FramePadding: s.style.framePadding = value; break;
                case StyleVar::ItemSpacing: s.style.itemSpacing = value; break;
                case StyleVar::WindowRounding:
                case StyleVar::WindowBorderSize:
                    break;
                }
            }
        }

        float windowRounding() {
            float rounding = 0.0F;
            for (const auto& [var, value] : state().varStack) {
                if (var == StyleVar::WindowRounding) {
                    rounding = value.x;
                }
            }
            return rounding;
        }

        QaplaUi::Font currentFont() {
            return state().fonts.empty() ? QaplaUi::Font::Inter : state().fonts.back();
        }

        float currentFontSize() {
            const float scale = state().stack.empty() ? 1.0F : current().fontScale;
            return baseFontSize(currentFont()) * scale;
        }

        float textLineHeight() {
            return lineHeight(currentFont(), currentFontSize());
        }

        float frameHeight() {
            return textLineHeight() + state().style.framePadding.y * 2.0F;
        }

        bool isDisabled() {
            return state().disabled > 0;
        }

        Color32 textColor() {
            const Color32 color = styleColor(StyleColor::Text);
            return isDisabled() ? scaleAlpha(color, DISABLED_ALPHA) : color;
        }

        // --------------------------------------------------------------------------------------
        // Input
        // --------------------------------------------------------------------------------------

        /** @brief A point in a window's base coordinates, in its content's top-down ones. */
        Vec2 toContent(NSWindow* window, NSPoint location) {
            NSView* content = window.contentView;
            NSPoint local = [content convertPoint:location fromView:nil];
            if (!content.isFlipped) {
                local.y = content.bounds.size.height - local.y;
            }
            return { static_cast<float>(local.x), static_cast<float>(local.y) };
        }

        /**
         * @brief Where the mouse is in a window, as its events said.
         *
         * Taken from the events, not from the cursor: an event posted by a script (see
         * QAPLA_UI_SCRIPT) moves no cursor, and the frame has to see it where it happened.
         */
        Vec2 mouseIn(NSWindow* window) {
            if (window == nil) {
                return {};
            }
            auto& positions = state().mouseByWindow;
            auto found = positions.find((__bridge void*)window);
            if (found != positions.end()) {
                return found->second;
            }
            const Vec2 position = toContent(window, [window mouseLocationOutsideOfEventStream]);
            positions[(__bridge void*)window] = position;
            return position;
        }

        NSView* hitViewIn(NSWindow* window) {
            auto& s = state();
            auto found = s.hitViews.find((__bridge void*)window);
            if (found != s.hitViews.end()) {
                return found->second;
            }
            NSView* content = window.contentView;
            const Vec2 mouse = mouseIn(window);
            NSPoint inContent = NSMakePoint(mouse.x, content.isFlipped ? mouse.y
                                                                       : content.bounds.size.height - mouse.y);
            NSPoint inSuper = content.superview != nil
                ? [content.superview convertPoint:inContent fromView:content] : inContent;
            NSView* hit = [content hitTest:inSuper];
            s.hitViews[(__bridge void*)window] = hit;
            return hit;
        }

        std::optional<QaplaUi::Key> keyFromCode(unsigned short code) {
            using QaplaUi::Key;
            switch (code) {
            case 126: return Key::UpArrow;
            case 125: return Key::DownArrow;
            case 123: return Key::LeftArrow;
            case 124: return Key::RightArrow;
            case 116: return Key::PageUp;
            case 121: return Key::PageDown;
            case 115: return Key::Home;
            case 119: return Key::End;
            case 36:
            case 76: return Key::Enter;
            case 53: return Key::Escape;
            case 117: return Key::Delete;
            case 51: return Key::Backspace;
            case 48: return Key::Tab;
            case 0: return Key::A;
            case 8: return Key::C;
            case 9: return Key::V;
            case 7: return Key::X;
            case 6: return Key::Z;
            default: return std::nullopt;
            }
        }

        bool editingText() {
            NSWindow* key = [NSApp keyWindow];
            NSResponder* responder = key.firstResponder;
            return [responder isKindOfClass:[NSTextView class]];
        }

        void processEvents() {
            auto& s = state();
            s.clicked = {};
            s.released = {};
            s.pressed.clear();
            std::array<bool, 3> transition{};
            while (!s.events.empty()) {
                const auto event = s.events.front();
                if (event.window != nil && event.type != InputEvent::Type::KeyDown &&
                    event.type != InputEvent::Type::KeyUp) {
                    // A button change waiting for the next frame keeps its position for it.
                    const bool deferred = (event.type == InputEvent::Type::MouseDown ||
                        event.type == InputEvent::Type::MouseUp) &&
                        transition[static_cast<std::size_t>(std::clamp(event.button, 0, 2))];
                    if (!deferred) {
                        s.mouseByWindow[(__bridge void*)event.window] = toContent(event.window, event.location);
                    }
                }
                if (event.type == InputEvent::Type::MouseMove) {
                    s.events.pop_front();
                    continue;
                }
                if (event.type == InputEvent::Type::MouseDown || event.type == InputEvent::Type::MouseUp) {
                    const auto button = static_cast<std::size_t>(std::clamp(event.button, 0, 2));
                    // One change of a button per frame, as ImGui trickles its input: a click
                    // whose press and release fall into the same frame is still seen as both.
                    if (transition[button]) {
                        break;
                    }
                    transition[button] = true;
                    if (event.type == InputEvent::Type::MouseDown) {
                        s.down[button] = true;
                        s.clicked[button] = true;
                    } else {
                        s.down[button] = false;
                        s.released[button] = true;
                    }
                    s.eventWindow[button] = event.window;
                } else if (event.type == InputEvent::Type::KeyDown) {
                    if (auto key = keyFromCode(event.keyCode)) {
                        s.pressed.push_back({ *key, event.repeat });
                        s.keysDown.insert(static_cast<int>(*key));
                    }
                } else {
                    if (auto key = keyFromCode(event.keyCode)) {
                        s.keysDown.erase(static_cast<int>(*key));
                    }
                }
                s.events.pop_front();
            }
            s.previousMouse = s.mouse;
            s.mouse = mouseIn(s.window);
            s.modifiers = [NSEvent modifierFlags];
            s.hitViews.clear();
        }

        // --------------------------------------------------------------------------------------
        // Containers
        // --------------------------------------------------------------------------------------

        void startContainer(Container& container) {
            const auto& s = state();
            container.lastFrame = s.frame;
            container.cursor = container.padding;
            container.indentX = 0.0F;
            container.lineY = container.padding.y;
            container.lineHeight = 0.0F;
            container.lineHasFrame = false;
            container.previousItemEnd = container.padding;
            container.sameLineRequested = false;
            container.contentMax = container.padding;
            container.nextItemWidth.reset();
            container.itemWidthStack.clear();
            container.groups.clear();
            container.elements.clear();
            container.runs.clear();
            container.tooltips.clear();
            container.keyUses.clear();
            container.lastItemRect = {};
            container.lastItemKey = 0;
            container.lastItemWidget = nullptr;
            container.lastItemHovered = false;
            container.lastItemDeactivated = false;
            container.lastItemActive = false;
            container.fontScale = 1.0F;
            container.drawList.bind(&container);
            if (container.scroll != nil) {
                container.scrollY = static_cast<float>(container.scroll.contentView.bounds.origin.y);
            }
        }

        QPContainerView* makeContainerView() {
            QPContainerView* view = [[QPContainerView alloc] initWithFrame:NSMakeRect(0, 0, 10, 10)];
            return view;
        }

        void makeScrollable(Container& container, bool horizontal) {
            QPScrollView* scroll = [[QPScrollView alloc] initWithFrame:NSMakeRect(0, 0, 10, 10)];
            scroll.drawsBackground = NO;
            scroll.hasVerticalScroller = YES;
            scroll.hasHorizontalScroller = horizontal;
            scroll.autohidesScrollers = YES;
            scroll.borderType = NSNoBorder;
            scroll.scrollerStyle = NSScrollerStyleOverlay;
            scroll.documentView = container.view;
            scroll.contentView.drawsBackground = NO;
            container.scroll = scroll;
            container.outer = scroll;
        }

        void destroyWidget(Widget& widget) {
            if (widget.kind == WidgetKind::Table) {
                releaseTableView(widget);
            }
            if ([widget.target isKindOfClass:[QPTarget class]]) {
                ((QPTarget*)widget.target).widget = nullptr;
            }
            if ([widget.view respondsToSelector:@selector(setWidget:)]) {
                [(id)widget.view setWidget:nullptr];
            }
            [widget.view removeFromSuperview];
            widget.view = nil;
            widget.control = nil;
            widget.extra = nil;
            widget.target = nil;
        }

        void destroyContainer(Container& container);

        void destroyChildren(Container& container) {
            for (auto& [key, widget] : container.widgets) {
                destroyWidget(*widget);
            }
            container.widgets.clear();
            for (auto& [key, child] : container.children) {
                destroyContainer(*child);
            }
            container.children.clear();
        }

        void destroyContainer(Container& container) {
            destroyChildren(container);
            if (container.panel != nil) {
                if (container.panel.sheetParent != nil) {
                    [container.panel.sheetParent endSheet:container.panel];
                }
                [container.panel orderOut:nil];
                container.panel = nil;
            }
            [container.outer removeFromSuperview];
            container.outer = nil;
            container.view = nil;
            container.scroll = nil;
        }

        std::vector<DrawCmd>& currentRun(Container& container) {
            if (container.elements.empty() || container.elements.back().run < 0) {
                container.runs.emplace_back();
                container.elements.push_back({ nil, static_cast<int>(container.runs.size()) - 1 });
            }
            return container.runs[static_cast<std::size_t>(container.elements.back().run)].commands;
        }

        void finishContainer(Container& container) {
            const int frame = state().frame;

            // Whatever no call asked for in this frame goes.
            for (auto it = container.widgets.begin(); it != container.widgets.end();) {
                if (it->second->lastFrame != frame) {
                    destroyWidget(*it->second);
                    it = container.widgets.erase(it);
                } else {
                    ++it;
                }
            }
            for (auto it = container.children.begin(); it != container.children.end();) {
                if (it->second->lastFrame != frame) {
                    destroyContainer(*it->second);
                    it = container.children.erase(it);
                } else {
                    ++it;
                }
            }

            // The size of what was laid out: the scrolled area, and the next auto-resize.
            const Vec2 contentSize = { container.contentMax.x + container.padding.x,
                                       container.contentMax.y + container.padding.y };
            container.lastContentSize = contentSize;
            NSSize docSize = NSMakeSize(container.size.x, container.size.y);
            if (container.scroll != nil) {
                docSize.height = std::max<CGFloat>(container.size.y, contentSize.y);
                if (container.horizontalScroll) {
                    docSize.width = std::max<CGFloat>(container.size.x, contentSize.x);
                }
            }
            if (!NSEqualSizes(container.view.frame.size, docSize)) {
                [container.view setFrameSize:docSize];
            }

            // The layers that paint the drawing, one per run, between the views.
            std::size_t runCount = container.runs.size();
            while (container.layers.size() < runCount) {
                QPLayerView* layer = [[QPLayerView alloc] initWithFrame:NSMakeRect(0, 0, 1, 1)];
                container.layers.push_back(layer);
            }
            NSMutableArray<NSView*>* order = [NSMutableArray arrayWithCapacity:container.elements.size()];
            const NSRect bounds = NSMakeRect(0, 0, docSize.width, docSize.height);
            for (const auto& element : container.elements) {
                if (element.run >= 0) {
                    const auto& commands = container.runs[static_cast<std::size_t>(element.run)].commands;
                    if (commands.empty()) {
                        continue;
                    }
                    QPLayerView* layer = container.layers[static_cast<std::size_t>(element.run)];
                    if (!NSEqualRects(layer.frame, bounds)) {
                        layer.frame = bounds;
                    }
                    [layer setCommands:commands];
                    [order addObject:layer];
                } else if (element.view != nil) {
                    [order addObject:element.view];
                }
            }
            if (![container.view.subviews isEqualToArray:order]) {
                container.view.subviews = order;
            }
            for (std::size_t index = runCount; index < container.layers.size(); ++index) {
                [container.layers[index] setCommands:{}];
            }

            if (container.tooltips != container.shownTooltips) {
                [container.view removeAllToolTips];
                container.tooltipStrings.clear();
                for (const auto& tip : container.tooltips) {
                    NSString* text = toNS(tip.text);
                    container.tooltipStrings.push_back(text);
                    [container.view addToolTipRect:tip.rect owner:text userData:nullptr];
                }
                container.shownTooltips = container.tooltips;
            }

            if (container.scroll != nil) {
                std::optional<float> target = container.requestedScrollY;
                if (container.requestedScrollRatio) {
                    target = *container.requestedScrollRatio;
                }
                if (target) {
                    const float maxScroll = std::max(0.0F, static_cast<float>(docSize.height) - container.size.y);
                    const float y = std::clamp(*target, 0.0F, maxScroll);
                    [container.scroll.contentView scrollToPoint:NSMakePoint(0, y)];
                    [container.scroll reflectScrolledClipView:container.scroll.contentView];
                }
                container.requestedScrollY.reset();
                container.requestedScrollRatio.reset();
            }

            if (container.background) {
                container.view.hasCustomBackground = YES;
                NSColor* color = nsColor(*container.background);
                if (![container.view.customBackground isEqual:color]) {
                    container.view.customBackground = color;
                    [container.view setNeedsDisplay:YES];
                }
                if (container.view.cornerRadius != container.rounding) {
                    container.view.cornerRadius = container.rounding;
                    [container.view setNeedsDisplay:YES];
                }
            } else if (container.view.hasCustomBackground) {
                container.view.hasCustomBackground = NO;
                [container.view setNeedsDisplay:YES];
            }
        }

        // --------------------------------------------------------------------------------------
        // Layout
        // --------------------------------------------------------------------------------------

        Vec2 cursorPos(const Container& container) {
            return container.sameLineRequested ? container.sameLinePos : container.cursor;
        }

        /**
         * @brief Places an item of a size at the cursor and moves the cursor on.
         * @param framed True for controls with a frame: text on their line is centred to them.
         * @return The item's rectangle in the container's coordinates.
         */
        ItemRect placeItem(Container& container, Vec2 size, bool framed) {
            const auto& style = state().style;
            Vec2 pos;
            if (container.sameLineRequested) {
                pos = container.sameLinePos;
                container.sameLineRequested = false;
            } else {
                pos = container.cursor;
                container.lineY = pos.y;
                container.lineHeight = 0.0F;
                container.lineHasFrame = false;
            }
            float offsetY = 0.0F;
            if (!framed && container.lineHasFrame) {
                offsetY = style.framePadding.y;
            }
            if (framed) {
                container.lineHasFrame = true;
            }
            const Vec2 itemPos = { pos.x, pos.y + offsetY };
            container.lineHeight = std::max(container.lineHeight, (itemPos.y - container.lineY) + size.y);
            container.previousItemEnd = { itemPos.x + size.x, container.lineY };
            container.cursor = { container.padding.x + container.indentX,
                                 container.lineY + container.lineHeight + style.itemSpacing.y };
            container.contentMax.x = std::max(container.contentMax.x, itemPos.x + size.x);
            container.contentMax.y = std::max(container.contentMax.y, itemPos.y + size.y);

            ItemRect rect{ itemPos, itemPos + size };
            const Vec2 origin = containerOrigin(container);
            container.lastItemRect = { rect.min + origin, rect.max + origin };
            container.lastItemWidget = nullptr;
            container.lastItemKey = 0;
            container.lastItemDeactivated = false;
            container.lastItemActive = false;
            container.lastItemHovered = false;
            return rect;
        }

        float availableWidth(const Container& container) {
            return container.size.x - container.padding.x - cursorPos(container).x;
        }

        float takeItemWidth(Container& container) {
            std::optional<float> width = container.nextItemWidth;
            container.nextItemWidth.reset();
            if (!width && !container.itemWidthStack.empty()) {
                width = container.itemWidthStack.back();
            }
            if (!width) {
                return std::max(1.0F, std::floor(container.size.x * 0.65F));
            }
            if (*width < 0.0F) {
                return std::max(1.0F, availableWidth(container) + *width);
            }
            return std::max(1.0F, *width);
        }

        NSRect toNSRect(const ItemRect& rect) {
            return NSMakeRect(rect.min.x, rect.min.y, rect.max.x - rect.min.x, rect.max.y - rect.min.y);
        }

        bool mouseOverContainer(const Container& container) {
            const auto& s = state();
            NSWindow* window = container.window != nil ? container.window : s.window;
            if (window != s.window && window != [NSApp keyWindow]) {
                return false;
            }
            NSView* hit = hitViewIn(window);
            return hit == container.view;
        }

        Vec2 mouseFor(const Container& container) {
            const auto& s = state();
            NSWindow* window = container.window != nil ? container.window : s.window;
            return window == s.window ? s.mouse : mouseIn(window);
        }

        bool clickedIn(const Container& container, std::size_t button) {
            const auto& s = state();
            NSWindow* window = container.window != nil ? container.window : s.window;
            return s.clicked[button] && s.eventWindow[button] == window;
        }

        // --------------------------------------------------------------------------------------
        // Widgets
        // --------------------------------------------------------------------------------------

        template <typename Make>
        Widget& obtain(Container& container, std::uint64_t key, WidgetKind kind, Make make) {
            auto& slot = container.widgets[key];
            if (slot && slot->kind != kind) {
                destroyWidget(*slot);
                slot.reset();
            }
            if (!slot) {
                slot = std::make_unique<Widget>();
                slot->kind = kind;
                slot->key = key;
                make(*slot);
                [container.view addSubview:slot->view];
            }
            slot->lastFrame = state().frame;
            return *slot;
        }

        void placeView(Container& container, Widget& widget, const ItemRect& rect) {
            const NSRect frame = toNSRect(rect);
            if (!NSEqualRects(frame, widget.shownFrame)) {
                widget.view.frame = frame;
                widget.shownFrame = frame;
            }
            container.elements.push_back({ widget.view, -1 });
            container.lastItemWidget = &widget;
            container.lastItemKey = widget.key;
            const bool enabled = !isDisabled();
            if (widget.shownEnabled != enabled) {
                widget.shownEnabled = enabled;
                if (widget.control != nil) {
                    widget.control.enabled = enabled;
                }
                if ([widget.extra isKindOfClass:[NSControl class]]) {
                    ((NSControl*)widget.extra).enabled = enabled;
                }
                if ([widget.view respondsToSelector:@selector(setEnabled:)] && widget.control == nil) {
                    [(id)widget.view setEnabled:enabled];
                }
            }
            NSView* hit = hitViewIn(container.window != nil ? container.window : state().window);
            container.lastItemHovered = hit != nil && [hit isDescendantOf:widget.view];
        }

        QPTarget* targetFor(Widget& widget) {
            QPTarget* target = [[QPTarget alloc] init];
            target.widget = &widget;
            widget.target = target;
            return target;
        }

        void setTitle(Widget& widget, NSButton* button, std::string_view title) {
            if (widget.shownTitle != title) {
                widget.shownTitle = std::string(title);
                button.title = toNS(title);
                widget.naturalSize = { static_cast<float>(button.intrinsicContentSize.width),
                                       static_cast<float>(button.intrinsicContentSize.height) };
            }
        }

        /** @brief Draws a control's label to its right, as ImGui places it. */
        void drawLabel(Container& container, const ItemRect& controlRect, std::string_view label) {
            auto shown = displayPart(label);
            if (shown.empty()) {
                return;
            }
            const auto& style = state().style;
            const Vec2 origin = containerOrigin(container);
            const float lineH = textLineHeight();
            const float y = controlRect.min.y + std::max(0.0F, (controlRect.max.y - controlRect.min.y - lineH) * 0.5F);
            DrawCmd cmd;
            cmd.type = DrawCmd::Type::Text;
            cmd.coords = { controlRect.max.x + style.itemSpacing.x * 0.5F, y, 0, 0, 0, 0 };
            cmd.color = textColor();
            cmd.font = currentFont();
            cmd.fontSize = currentFontSize();
            cmd.text = std::string(shown);
            (void)origin;
            currentRun(container).push_back(std::move(cmd));
        }

        /** @brief Width a label takes beside its control, including the gap. */
        float labelWidth(std::string_view label) {
            auto shown = displayPart(label);
            if (shown.empty()) {
                return 0.0F;
            }
            return state().style.itemSpacing.x * 0.5F + measureText(currentFont(), currentFontSize(), shown).x;
        }

        NSTextField* makeTextField(Widget& widget) {
            NSTextField* field = [NSTextField textFieldWithString:@""];
            QPTarget* target = targetFor(widget);
            field.delegate = target;
            field.target = target;
            field.action = @selector(textEntered:);
            field.font = [NSFont systemFontOfSize:13];
            field.usesSingleLineMode = YES;
            field.cell.scrollable = YES;
            field.cell.wraps = NO;
            field.bezelStyle = NSTextFieldSquareBezel;
            return field;
        }

        void setFieldText(Widget& widget, NSTextField* field, const std::string& text) {
            if (widget.shownText != text) {
                field.stringValue = toNS(text);
                widget.shownText = text;
            }
        }

        // --------------------------------------------------------------------------------------
        // Number fields
        // --------------------------------------------------------------------------------------

        enum class NumberKind { Int, Float, U32 };

        std::string formatNumber(double value, NumberKind kind, const char* format) {
            char buffer[64];
            switch (kind) {
            case NumberKind::Int:
                std::snprintf(buffer, sizeof(buffer), format != nullptr ? format : "%d", static_cast<int>(value));
                break;
            case NumberKind::U32:
                std::snprintf(buffer, sizeof(buffer), "%u", static_cast<unsigned>(value));
                break;
            case NumberKind::Float:
                std::snprintf(buffer, sizeof(buffer), format != nullptr ? format : "%.3f", value);
                break;
            }
            return buffer;
        }

        std::optional<double> parseNumber(const std::string& text, NumberKind kind) {
            if (text.empty()) {
                return std::nullopt;
            }
            char* end = nullptr;
            const double value = std::strtod(text.c_str(), &end);
            if (end == text.c_str()) {
                return std::nullopt;
            }
            if (kind == NumberKind::U32 && value < 0) {
                return std::nullopt;
            }
            return kind == NumberKind::Float ? value : std::trunc(value);
        }

        bool numberField(std::string_view label, double& value, double step, NumberKind kind,
            const char* format) {
            auto& container = current();
            const auto key = uniqueKey(container, keyFor(label));
            auto& widget = obtain(container, key, WidgetKind::NumberField, [](Widget& w) {
                NSView* box = [[QPContainerView alloc] initWithFrame:NSMakeRect(0, 0, 100, 22)];
                NSTextField* field = makeTextField(w);
                NSStepper* stepper = [[NSStepper alloc] initWithFrame:NSMakeRect(0, 0, 15, 22)];
                stepper.minValue = -1.0e15;
                stepper.maxValue = 1.0e15;
                stepper.valueWraps = NO;
                stepper.autorepeat = YES;
                stepper.target = w.target;
                stepper.action = @selector(stepperChanged:);
                [box addSubview:field];
                [box addSubview:stepper];
                w.view = box;
                w.control = field;
                w.extra = stepper;
            });
            NSTextField* field = (NSTextField*)widget.control;
            NSStepper* stepper = (NSStepper*)widget.extra;

            const float width = takeItemWidth(container);
            const ItemRect rect = placeItem(container, { width + labelWidth(label), frameHeight() }, true);
            const ItemRect controlRect = { rect.min, { rect.min.x + width, rect.max.y } };
            placeView(container, widget, controlRect);

            const bool showStepper = step > 0.0;
            const float stepperWidth = showStepper ? 15.0F : 0.0F;
            const NSRect fieldFrame = NSMakeRect(0, 0, std::max(10.0F, width - stepperWidth - (showStepper ? 2.0F : 0.0F)), frameHeight());
            if (!NSEqualRects(field.frame, fieldFrame)) {
                field.frame = fieldFrame;
                stepper.frame = NSMakeRect(fieldFrame.size.width + 2.0, 0, stepperWidth, frameHeight());
            }
            stepper.hidden = !showStepper;
            stepper.increment = showStepper ? step : 1.0;

            bool changed = false;
            if (widget.stepped) {
                widget.stepped = false;
                value = widget.number;
                changed = true;
            }
            if (widget.changed) {
                widget.changed = false;
                if (auto parsed = parseNumber(widget.text, kind)) {
                    widget.shownText = widget.text;
                    if (*parsed != value) {
                        value = *parsed;
                        changed = true;
                    }
                }
            }
            const bool editing = field.currentEditor != nil;
            const std::string shown = formatNumber(value, kind, format);
            if (!editing) {
                setFieldText(widget, field, shown);
            }
            if (stepper.doubleValue != value) {
                stepper.doubleValue = value;
            }
            container.lastItemDeactivated = widget.deactivated;
            widget.deactivated = false;
            drawLabel(container, controlRect, label);
            container.lastItemRect.max.x = containerOrigin(container).x + rect.max.x;
            return changed;
        }

        // --------------------------------------------------------------------------------------
        // Popups
        // --------------------------------------------------------------------------------------

        void closeOutsidePopups() {
            auto& s = state();
            if (!s.clicked[0] && !s.clicked[1]) {
                return;
            }
            for (auto& [key, container] : s.popupContainers) {
                auto found = s.popups.find(key);
                if (found == s.popups.end() || !found->second.open || found->second.openedFrame == s.frame) {
                    continue;
                }
                const Vec2 origin = containerOrigin(*container);
                const ItemRect rect{ origin, origin + container->size };
                if (!rect.contains(s.mouse)) {
                    found->second.open = false;
                }
            }
        }

        void closePopup(std::uint64_t key, bool modal) {
            auto& s = state();
            s.popups[key].open = false;
            if (modal) {
                auto found = s.modals.find(key);
                if (found != s.modals.end() && found->second->panel != nil) {
                    NSPanel* panel = found->second->panel;
                    if (panel.sheetParent != nil) {
                        [panel.sheetParent endSheet:panel];
                    }
                    [panel orderOut:nil];
                }
            }
            scheduleFrame();
        }

        Container& beginTopLevel(std::unordered_map<std::uint64_t, std::unique_ptr<Container>>& map,
            std::uint64_t key, ContainerKind kind) {
            auto& slot = map[key];
            if (!slot) {
                slot = std::make_unique<Container>();
                slot->kind = kind;
                slot->key = key;
                slot->view = makeContainerView();
                slot->outer = slot->view;
                slot->window = state().window;
            }
            return *slot;
        }

        void pushContainer(Container& container) {
            state().stack.push_back(&container);
            startContainer(container);
        }

        void popContainer() {
            auto& s = state();
            Container* container = s.stack.back();
            finishContainer(*container);
            s.stack.pop_back();
        }

    } // namespace

    // ------------------------------------------------------------------------------------------
    // Frame
    // ------------------------------------------------------------------------------------------

    void appendDrawCmd(Container& container, DrawCmd&& cmd) {
        currentRun(container).push_back(std::move(cmd));
    }

    Vec2 containerOrigin(const Container& container) {
        return container.originInWindow;
    }

    void recordEvent(NSEvent* event) {
        auto& s = state();
        InputEvent record{};
        record.window = event.window;
        record.location = event.locationInWindow;
        switch (event.type) {
        case NSEventTypeMouseMoved:
        case NSEventTypeLeftMouseDragged:
        case NSEventTypeRightMouseDragged:
        case NSEventTypeOtherMouseDragged:
            record.type = InputEvent::Type::MouseMove;
            // Only the last of a run of moves matters.
            if (!s.events.empty() && s.events.back().type == InputEvent::Type::MouseMove &&
                s.events.back().window == event.window) {
                s.events.back() = record;
            } else {
                s.events.push_back(record);
            }
            s.fastUntil = CACurrentMediaTime() + 1.0;
            scheduleFrame();
            return;
        case NSEventTypeLeftMouseDown:
            record.type = InputEvent::Type::MouseDown;
            record.button = 0;
            break;
        case NSEventTypeLeftMouseUp:
            record.type = InputEvent::Type::MouseUp;
            record.button = 0;
            break;
        case NSEventTypeRightMouseDown:
            record.type = InputEvent::Type::MouseDown;
            record.button = 1;
            break;
        case NSEventTypeRightMouseUp:
            record.type = InputEvent::Type::MouseUp;
            record.button = 1;
            break;
        case NSEventTypeOtherMouseDown:
            record.type = InputEvent::Type::MouseDown;
            record.button = 2;
            break;
        case NSEventTypeOtherMouseUp:
            record.type = InputEvent::Type::MouseUp;
            record.button = 2;
            break;
        case NSEventTypeKeyDown:
            record.type = InputEvent::Type::KeyDown;
            record.keyCode = event.keyCode;
            record.repeat = event.isARepeat;
            break;
        case NSEventTypeKeyUp:
            record.type = InputEvent::Type::KeyUp;
            record.keyCode = event.keyCode;
            break;
        default:
            s.fastUntil = CACurrentMediaTime() + 0.5;
            return;
        }
        s.events.push_back(record);
        s.fastUntil = CACurrentMediaTime() + 1.0;
        scheduleFrame();
    }

    void setFrameScheduler(void (*scheduler)()) {
        state().scheduler = scheduler;
    }

    void scheduleFrame() {
        auto& s = state();
        s.fastUntil = std::max(s.fastUntil, CACurrentMediaTime() + 0.25);
        if (s.frameScheduled || s.scheduler == nullptr) {
            return;
        }
        s.frameScheduled = true;
        dispatch_async(dispatch_get_main_queue(), ^{
            state().frameScheduled = false;
            if (state().scheduler != nullptr) {
                state().scheduler();
            }
        });
    }

    bool wantsFastFrames() {
        return CACurrentMediaTime() < state().fastUntil;
    }

    NSView* rootView() {
        return state().root;
    }

    void beginFrame(NSWindow* window) {
        auto& s = state();
        s.inFrame = true;
        s.window = window;
        s.root = (QPRootView*)window.contentView;
        const double now = CACurrentMediaTime();
        if (s.frame == 0) {
            s.startTime = now;
            s.frameTime = now;
        }
        s.deltaTime = static_cast<float>(std::max(1.0e-4, now - s.frameTime));
        s.frameTime = now;
        ++s.frame;

        processEvents();
        closeOutsidePopups();

        s.stack.clear();
        s.idStack.clear();
        s.overlayOrder.clear();
        s.popupOrder.clear();
        s.fonts.clear();
        for (auto& stack : s.colorStacks) {
            stack.clear();
        }
        s.colorOrder.clear();
        s.disabledStack.clear();
        s.varStack.clear();
        applyVars();
        s.disabled = 0;
        s.wrapStack.clear();
        s.combo = nullptr;
        s.tabBars.clear();
        s.popupScopes.clear();

        // The implicit window anything lands in that no overlay was opened for.
        if (!s.implicitRoot) {
            s.implicitRoot = std::make_unique<Container>();
            s.implicitRoot->kind = ContainerKind::Overlay;
            s.implicitRoot->view = makeContainerView();
            s.implicitRoot->outer = s.implicitRoot->view;
        }
        auto& root = *s.implicitRoot;
        root.window = window;
        if (root.outer.superview != s.root) {
            [s.root addSubview:root.outer];
        }
        const NSRect bounds = s.root.bounds;
        if (!NSEqualRects(root.outer.frame, bounds)) {
            root.outer.frame = bounds;
        }
        root.size = { static_cast<float>(bounds.size.width), static_cast<float>(bounds.size.height) };
        root.originInWindow = {};
        root.padding = s.style.windowPadding;
        pushContainer(root);
    }

    void endFrame() {
        auto& s = state();
        while (s.stack.size() > 1) {
            popContainer();
        }
        if (!s.stack.empty()) {
            popContainer();
        }

        // Overlays, popups and dialogs nobody described in this frame go.
        auto sweep = [&s](auto& map) {
            for (auto it = map.begin(); it != map.end();) {
                if (it->second->lastFrame != s.frame) {
                    destroyContainer(*it->second);
                    it = map.erase(it);
                } else {
                    ++it;
                }
            }
        };
        sweep(s.overlays);
        sweep(s.popupContainers);
        sweep(s.modals);

        // The window's own z-order: the implicit root, the overlays in the order they came,
        // the popups above them all.
        NSMutableArray<NSView*>* order = [NSMutableArray array];
        if (s.implicitRoot) {
            [order addObject:s.implicitRoot->outer];
        }
        for (auto* overlay : s.overlayOrder) {
            [order addObject:overlay->outer];
        }
        for (auto* popup : s.popupOrder) {
            [order addObject:popup->outer];
        }
        if (![s.root.subviews isEqualToArray:order]) {
            s.root.subviews = order;
        }

        if (!s.down[0]) {
            s.activeKey = 0;
        }
        s.inFrame = false;
    }

} // namespace QaplaMac

// ----------------------------------------------------------------------------------------------
// QaplaUi
// ----------------------------------------------------------------------------------------------

namespace QaplaUi {

    using namespace QaplaMac;

    Color32 toColor32(const Vec4& color) {
        auto channel = [](float value) {
            return static_cast<std::uint32_t>(std::clamp(value, 0.0F, 1.0F) * 255.0F + 0.5F);
        };
        return col32(channel(color.x), channel(color.y), channel(color.z), channel(color.w));
    }

    Vec4 toVec4(Color32 color) {
        return { static_cast<float>(color & 0xFFU) / 255.0F,
                 static_cast<float>((color >> 8U) & 0xFFU) / 255.0F,
                 static_cast<float>((color >> 16U) & 0xFFU) / 255.0F,
                 static_cast<float>((color >> 24U) & 0xFFU) / 255.0F };
    }

    DrawList& windowDrawList() {
        return current().drawList;
    }

    // Layout

    void sameLine(float offsetFromStartX, float spacing) {
        auto& container = current();
        container.sameLineRequested = true;
        if (offsetFromStartX > 0.0F) {
            container.sameLinePos = { offsetFromStartX, container.lineY };
        } else {
            container.sameLinePos = { container.previousItemEnd.x +
                (spacing < 0.0F ? state().style.itemSpacing.x : spacing), container.lineY };
        }
    }

    void newLine() {
        auto& container = current();
        if (container.sameLineRequested) {
            container.sameLineRequested = false;
            return;
        }
        placeItem(container, { 0.0F, textLineHeight() }, false);
    }

    void spacing() {
        placeItem(current(), { 0.0F, 0.0F }, false);
    }

    void separator() {
        auto& container = current();
        const Vec2 pos = cursorPos(container);
        const float width = std::max(1.0F, container.size.x - container.padding.x - pos.x);
        const ItemRect rect = placeItem(container, { width, 1.0F }, false);
        DrawCmd cmd;
        cmd.type = DrawCmd::Type::RectFilled;
        cmd.coords = { rect.min.x, rect.min.y, rect.max.x, rect.min.y + 1.0F, 0, 0 };
        cmd.color = styleColor(StyleColor::Separator);
        appendDrawCmd(container, std::move(cmd));
    }

    void indent(float width) {
        auto& container = current();
        const float amount = width > 0.0F ? width : 21.0F;
        container.indentX += amount;
        if (!container.sameLineRequested) {
            container.cursor.x = container.padding.x + container.indentX;
        }
    }

    void unindent(float width) {
        auto& container = current();
        const float amount = width > 0.0F ? width : 21.0F;
        container.indentX -= amount;
        if (!container.sameLineRequested) {
            container.cursor.x = container.padding.x + container.indentX;
        }
    }

    void dummy(Vec2 size) {
        placeItem(current(), size, false);
    }

    void beginGroup() {
        auto& container = current();
        const Vec2 start = cursorPos(container);
        container.groups.push_back({ start, container.indentX, container.contentMax });
        container.sameLineRequested = false;
        container.cursor = start;
        container.indentX = start.x - container.padding.x;
        container.contentMax = start;
    }

    void endGroup() {
        auto& container = current();
        if (container.groups.empty()) {
            return;
        }
        const auto group = container.groups.back();
        container.groups.pop_back();
        const Vec2 groupMax = container.contentMax;
        container.indentX = group.indentX;
        container.contentMax = { std::max(group.contentMax.x, groupMax.x),
                                 std::max(group.contentMax.y, groupMax.y) };
        container.sameLineRequested = true;
        container.sameLinePos = group.start;
        container.lineY = group.start.y;
        container.lineHeight = 0.0F;
        placeItem(container, { groupMax.x - group.start.x, groupMax.y - group.start.y }, false);
    }

    void setNextItemWidth(float width) { current().nextItemWidth = width; }
    void pushItemWidth(float width) { current().itemWidthStack.push_back(width); }
    void popItemWidth() {
        auto& stack = current().itemWidthStack;
        if (!stack.empty()) {
            stack.pop_back();
        }
    }
    float calcItemWidth() {
        auto& container = current();
        auto saved = container.nextItemWidth;
        const float width = takeItemWidth(container);
        container.nextItemWidth = saved;
        return width;
    }

    Vec2 getContentRegionAvail() {
        auto& container = current();
        const Vec2 pos = cursorPos(container);
        return { container.size.x - container.padding.x - pos.x,
                 container.size.y + container.scrollY - container.padding.y - pos.y };
    }
    Vec2 getCursorPos() { return cursorPos(current()); }
    float getCursorPosX() { return cursorPos(current()).x; }
    float getCursorPosY() { return cursorPos(current()).y; }
    void setCursorPos(Vec2 position) {
        auto& container = current();
        container.sameLineRequested = false;
        container.cursor = position;
        container.contentMax.x = std::max(container.contentMax.x, position.x);
        container.contentMax.y = std::max(container.contentMax.y, position.y);
    }
    void setCursorPosX(float x) {
        auto& container = current();
        if (container.sameLineRequested) {
            container.sameLinePos.x = x;
        } else {
            container.cursor.x = x;
        }
    }
    void setCursorPosY(float y) {
        auto& container = current();
        if (container.sameLineRequested) {
            container.sameLinePos.y = y;
            container.lineY = y;
        } else {
            container.cursor.y = y;
        }
        container.contentMax.y = std::max(container.contentMax.y, y);
    }
    Vec2 getCursorScreenPos() {
        auto& container = current();
        return cursorPos(container) + containerOrigin(container);
    }
    void setCursorScreenPos(Vec2 position) {
        setCursorPos(position - containerOrigin(current()));
    }

    float getFrameHeight() { return frameHeight(); }
    float getTextLineHeight() { return textLineHeight(); }
    float getTextLineHeightWithSpacing() { return textLineHeight() + state().style.itemSpacing.y; }
    float getFontSize() { return currentFontSize(); }

    Vec2 calcTextSize(std::string_view text, float wrapWidth) {
        return measureText(currentFont(), currentFontSize(), text, wrapWidth);
    }

    Vec2 calcTextSize(Font font, float size, std::string_view text, float wrapWidth) {
        return measureText(font, size, text, wrapWidth);
    }

    const Style& getStyle() { return state().style; }

    Vec2 getViewportSize() {
        NSRect bounds = state().root != nil ? state().root.bounds : NSZeroRect;
        return { static_cast<float>(bounds.size.width), static_cast<float>(bounds.size.height) };
    }
    Vec2 getViewportPos() { return {}; }

    // Identifiers and stacks

    void pushId(std::string_view id) {
        state().idStack.push_back(hashBytes(seed(), id.data(), id.size()));
    }
    void pushId(int id) {
        state().idStack.push_back(hashBytes(seed(), &id, sizeof(id)));
    }
    void pushId(const void* id) {
        state().idStack.push_back(hashBytes(seed(), &id, sizeof(id)));
    }
    void popId() {
        if (!state().idStack.empty()) {
            state().idStack.pop_back();
        }
    }

    void pushStyleColor(StyleColor which, Color32 color) {
        state().colorStacks[static_cast<std::size_t>(which)].push_back(color);
        state().colorOrder.push_back(which);
    }
    void pushStyleColor(StyleColor which, const Vec4& color) {
        pushStyleColor(which, toColor32(color));
    }
    void popStyleColor(int count) {
        // Popped in the reverse order they were pushed, whatever color each one was.
        auto& s = state();
        for (int index = 0; index < count && !s.colorOrder.empty(); ++index) {
            auto& stack = s.colorStacks[static_cast<std::size_t>(s.colorOrder.back())];
            if (!stack.empty()) {
                stack.pop_back();
            }
            s.colorOrder.pop_back();
        }
    }
    void pushStyleVar(StyleVar which, float value) {
        state().varStack.emplace_back(which, Vec2(value, value));
        applyVars();
    }
    void pushStyleVar(StyleVar which, Vec2 value) {
        state().varStack.emplace_back(which, value);
        applyVars();
    }
    void popStyleVar(int count) {
        auto& stack = state().varStack;
        for (int index = 0; index < count && !stack.empty(); ++index) {
            stack.pop_back();
        }
        applyVars();
    }
    Color32 getColorU32(StyleColor which, float alphaMultiplier) {
        float factor = alphaMultiplier * (isDisabled() ? DISABLED_ALPHA : 1.0F);
        return scaleAlpha(styleColor(which), factor);
    }
    Color32 getColorU32(const Vec4& color) {
        const Color32 packed = toColor32(color);
        return isDisabled() ? scaleAlpha(packed, DISABLED_ALPHA) : packed;
    }
    Vec4 getStyleColorVec4(StyleColor which) { return toVec4(styleColor(which)); }

    void pushFont(Font font) { state().fonts.push_back(font); }
    void popFont() {
        if (!state().fonts.empty()) {
            state().fonts.pop_back();
        }
    }
    Font getFont() { return currentFont(); }
    void setWindowFontScale(float scale) { current().fontScale = scale; }

    void beginDisabled(bool disabled) {
        // Kept even when false, so endDisabled() always has its partner.
        auto& s = state();
        s.disabledStack.push_back(disabled);
        s.disabled += disabled ? 1 : 0;
    }
    void endDisabled() {
        auto& s = state();
        if (s.disabledStack.empty()) {
            return;
        }
        if (s.disabledStack.back()) {
            s.disabled = std::max(0, s.disabled - 1);
        }
        s.disabledStack.pop_back();
    }

    void pushTextWrapPos(float wrapPosX) { state().wrapStack.push_back(wrapPosX); }
    void popTextWrapPos() {
        if (!state().wrapStack.empty()) {
            state().wrapStack.pop_back();
        }
    }

    // Text

    namespace {
        std::optional<float> wrapWidth(const Container& container) {
            const auto& stack = state().wrapStack;
            // As ImGui reads it: negative is no wrapping, 0 the container's edge, else that x.
            if (stack.empty() || stack.back() < 0.0F) {
                return std::nullopt;
            }
            const float pos = cursorPos(container).x;
            const float edge = stack.back() == 0.0F ? container.size.x - container.padding.x : stack.back();
            return std::max(1.0F, edge - pos);
        }

        void textWithColor(Color32 color, std::string_view text) {
            auto& container = current();
            const auto wrap = wrapWidth(container);
            const Vec2 size = measureText(currentFont(), currentFontSize(), text, wrap.value_or(-1.0F));
            const ItemRect rect = placeItem(container, size, false);
            DrawCmd cmd;
            cmd.type = DrawCmd::Type::Text;
            cmd.coords = { rect.min.x, rect.min.y, 0, 0, 0, 0 };
            cmd.color = color;
            cmd.font = currentFont();
            cmd.fontSize = currentFontSize();
            cmd.wrapWidth = wrap.value_or(0.0F);
            cmd.text = std::string(text);
            appendDrawCmd(container, std::move(cmd));
            container.lastItemHovered = mouseOverContainer(container) &&
                container.lastItemRect.contains(mouseFor(container));
        }
    }

    void text(std::string_view text) { textWithColor(textColor(), text); }
    void textColored(const Vec4& color, std::string_view text) {
        Color32 packed = toColor32(color);
        textWithColor(isDisabled() ? scaleAlpha(packed, DISABLED_ALPHA) : packed, text);
    }
    void textDisabled(std::string_view text) {
        textWithColor(scaleAlpha(styleColor(StyleColor::TextDisabled), isDisabled() ? DISABLED_ALPHA : 1.0F), text);
    }
    void textWrapped(std::string_view text) {
        pushTextWrapPos(0.0F);
        QaplaUi::text(text);
        popTextWrapPos();
    }
    void bullet() {
        auto& container = current();
        const float height = textLineHeight();
        const ItemRect rect = placeItem(container, { height, height }, false);
        DrawCmd cmd;
        cmd.type = DrawCmd::Type::CircleFilled;
        cmd.coords = { rect.min.x + height * 0.5F, rect.min.y + height * 0.5F, height * 0.18F, 0, 0, 0 };
        cmd.color = textColor();
        appendDrawCmd(container, std::move(cmd));
        sameLine(0.0F, state().style.framePadding.x * 2.0F);
    }

    // Controls

    bool button(std::string_view label, Vec2 size) {
        auto& container = current();
        const auto key = uniqueKey(container, keyFor(label));
        auto& widget = obtain(container, key, WidgetKind::Button, [](Widget& w) {
            NSButton* button = [NSButton buttonWithTitle:@"" target:nil action:@selector(buttonClicked:)];
            button.target = targetFor(w);
            button.font = [NSFont systemFontOfSize:13];
            w.view = button;
            w.control = button;
        });
        NSButton* button = (NSButton*)widget.control;
        setTitle(widget, button, displayPart(label));

        const float avail = availableWidth(container);
        // A size the caller computed from its text is a minimum here: a native button needs
        // more room around its title than an ImGui one, and a cut title helps nobody.
        float width = size.x > 0.0F ? std::max(size.x, widget.naturalSize.x)
                                    : (size.x < 0.0F ? std::max(1.0F, avail + size.x) : widget.naturalSize.x);
        float height = size.y > 0.0F ? size.y : frameHeight();
        const NSBezelStyle bezel = height > 26.0F ? NSBezelStyleFlexiblePush : NSBezelStylePush;
        if (button.bezelStyle != bezel) {
            button.bezelStyle = bezel;
        }
        const ItemRect rect = placeItem(container, { width, height }, true);
        placeView(container, widget, rect);
        const bool clicked = widget.clicked && !isDisabled();
        widget.clicked = false;
        return clicked;
    }

    bool smallButton(std::string_view label) {
        auto& container = current();
        const auto key = uniqueKey(container, keyFor(label));
        auto& widget = obtain(container, key, WidgetKind::Button, [](Widget& w) {
            NSButton* button = [NSButton buttonWithTitle:@"" target:nil action:@selector(buttonClicked:)];
            button.target = targetFor(w);
            button.controlSize = NSControlSizeSmall;
            button.font = [NSFont systemFontOfSize:11];
            w.view = button;
            w.control = button;
        });
        NSButton* button = (NSButton*)widget.control;
        setTitle(widget, button, displayPart(label));
        const ItemRect rect = placeItem(container, { widget.naturalSize.x, textLineHeight() + 2.0F }, false);
        placeView(container, widget, rect);
        const bool clicked = widget.clicked && !isDisabled();
        widget.clicked = false;
        return clicked;
    }

    bool invisibleButton(std::string_view id, Vec2 size) {
        auto& s = state();
        auto& container = current();
        const auto key = uniqueKey(container, keyFor(id));
        placeItem(container, { std::max(1.0F, size.x), std::max(1.0F, size.y) }, false);
        container.lastItemKey = key;
        const bool hovered = !isDisabled() && mouseOverContainer(container) &&
            container.lastItemRect.contains(mouseFor(container));
        container.lastItemHovered = hovered;
        if (hovered && clickedIn(container, 0)) {
            s.activeKey = key;
        }
        bool pressed = false;
        if (s.activeKey == key) {
            if (s.released[0]) {
                pressed = hovered;
            }
        }
        container.lastItemActive = s.activeKey == key && (s.down[0] || s.released[0]);
        return pressed;
    }

    bool checkbox(std::string_view label, bool& value) {
        auto& container = current();
        const auto key = uniqueKey(container, keyFor(label));
        auto& widget = obtain(container, key, WidgetKind::Checkbox, [](Widget& w) {
            NSButton* box = [NSButton checkboxWithTitle:@"" target:nil action:@selector(checkboxToggled:)];
            box.target = targetFor(w);
            box.font = [NSFont systemFontOfSize:13];
            w.view = box;
            w.control = box;
        });
        NSButton* box = (NSButton*)widget.control;
        setTitle(widget, box, displayPart(label));
        bool changed = false;
        if (widget.changed) {
            widget.changed = false;
            value = widget.number > 0.5;
            changed = true;
        }
        const NSControlStateValue wanted = value ? NSControlStateValueOn : NSControlStateValueOff;
        if (box.state != wanted) {
            box.state = wanted;
        }
        const float width = std::max(18.0F, widget.naturalSize.x);
        const ItemRect rect = placeItem(container, { width, frameHeight() }, true);
        placeView(container, widget, rect);
        return changed;
    }

    bool radioButton(std::string_view label, bool active) {
        auto& container = current();
        const auto key = uniqueKey(container, keyFor(label));
        auto& widget = obtain(container, key, WidgetKind::Radio, [](Widget& w) {
            NSButton* radio = [NSButton radioButtonWithTitle:@"" target:nil action:@selector(buttonClicked:)];
            radio.target = targetFor(w);
            radio.font = [NSFont systemFontOfSize:13];
            w.view = radio;
            w.control = radio;
        });
        NSButton* radio = (NSButton*)widget.control;
        setTitle(widget, radio, displayPart(label));
        const NSControlStateValue wanted = active ? NSControlStateValueOn : NSControlStateValueOff;
        if (radio.state != wanted) {
            radio.state = wanted;
        }
        const ItemRect rect = placeItem(container, { std::max(18.0F, widget.naturalSize.x), frameHeight() }, true);
        placeView(container, widget, rect);
        const bool clicked = widget.clicked && !isDisabled();
        widget.clicked = false;
        return clicked;
    }

    bool inputText(std::string_view label, std::string& value, InputTextFlags flags,
        const CharFilter& filter) {
        auto& container = current();
        const auto key = uniqueKey(container, keyFor(label));
        auto& widget = obtain(container, key, WidgetKind::TextField, [](Widget& w) {
            NSTextField* field = makeTextField(w);
            w.view = field;
            w.control = field;
        });
        NSTextField* field = (NSTextField*)widget.control;
        const bool readOnly = hasFlag(flags, InputTextFlags::ReadOnly);
        if (field.editable == readOnly) {
            field.editable = !readOnly;
        }

        const float width = takeItemWidth(container);
        const ItemRect rect = placeItem(container, { width + labelWidth(label), frameHeight() }, true);
        const ItemRect controlRect = { rect.min, { rect.min.x + width, rect.max.y } };
        placeView(container, widget, controlRect);

        bool result = false;
        if (widget.changed) {
            widget.changed = false;
            std::string text = widget.text;
            if (filter) {
                std::string filtered;
                NSString* string = toNS(text);
                for (NSUInteger index = 0; index < string.length; ++index) {
                    const unichar character = [string characterAtIndex:index];
                    if (filter(character)) {
                        filtered += fromNS([NSString stringWithCharacters:&character length:1]);
                    }
                }
                if (filtered != text) {
                    text = filtered;
                    field.stringValue = toNS(text);
                }
            }
            widget.shownText = text;
            if (value != text) {
                value = text;
                result = !hasFlag(flags, InputTextFlags::EnterReturnsTrue);
            }
        }
        if (widget.enterPressed) {
            widget.enterPressed = false;
            if (hasFlag(flags, InputTextFlags::EnterReturnsTrue)) {
                result = true;
            }
        }
        // The application may have changed the value itself -- cleared it, normalised it.
        if (widget.shownText != value) {
            field.stringValue = toNS(value);
            widget.shownText = value;
        }
        container.lastItemDeactivated = widget.deactivated;
        widget.deactivated = false;
        drawLabel(container, controlRect, label);
        container.lastItemRect.max.x = containerOrigin(container).x + rect.max.x;
        return result;
    }

    bool inputTextMultiline(std::string_view label, std::string& value, Vec2 size,
        InputTextFlags flags) {
        auto& container = current();
        const auto key = uniqueKey(container, keyFor(label));
        auto& widget = obtain(container, key, WidgetKind::MultilineText, [](Widget& w) {
            NSScrollView* scroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 200, 80)];
            scroll.hasVerticalScroller = YES;
            scroll.borderType = NSBezelBorder;
            NSTextView* textView = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, 200, 80)];
            textView.richText = NO;
            textView.font = [NSFont systemFontOfSize:13];
            textView.automaticQuoteSubstitutionEnabled = NO;
            textView.automaticDashSubstitutionEnabled = NO;
            textView.automaticTextReplacementEnabled = NO;
            textView.verticallyResizable = YES;
            textView.horizontallyResizable = NO;
            textView.autoresizingMask = NSViewWidthSizable;
            textView.textContainer.widthTracksTextView = YES;
            textView.delegate = targetFor(w);
            scroll.documentView = textView;
            w.view = scroll;
            w.extra = textView;
        });
        NSTextView* textView = (NSTextView*)widget.extra;
        widget.submitOnEnter = hasFlag(flags, InputTextFlags::CtrlEnterForNewLine) &&
            hasFlag(flags, InputTextFlags::EnterReturnsTrue);
        textView.editable = !hasFlag(flags, InputTextFlags::ReadOnly) && !isDisabled();

        const float avail = availableWidth(container);
        const float width = size.x > 0.0F ? size.x : (size.x < 0.0F ? std::max(1.0F, avail + size.x + 1.0F)
                                                                     : takeItemWidth(container));
        const float height = size.y > 0.0F ? size.y : textLineHeight() * 8.0F;
        const ItemRect rect = placeItem(container, { width, height }, true);
        placeView(container, widget, rect);

        bool result = false;
        if (widget.changed) {
            widget.changed = false;
            widget.shownText = widget.text;
            if (value != widget.text) {
                value = widget.text;
                result = !hasFlag(flags, InputTextFlags::EnterReturnsTrue);
            }
        }
        if (widget.enterPressed) {
            widget.enterPressed = false;
            value = widget.text;
            widget.shownText = value;
            result = true;
        }
        if (widget.shownText != value) {
            textView.string = toNS(value);
            widget.shownText = value;
            widget.text = value;
        }
        return result;
    }

    bool inputInt(std::string_view label, int& value, int step, int stepFast) {
        static_cast<void>(stepFast);
        double number = value;
        const bool changed = numberField(label, number, step, NumberKind::Int, "%d");
        if (changed) {
            value = static_cast<int>(std::clamp<double>(number, std::numeric_limits<int>::min(),
                std::numeric_limits<int>::max()));
        }
        return changed;
    }

    bool inputFloat(std::string_view label, float& value, float step, float stepFast,
        const char* format) {
        static_cast<void>(stepFast);
        double number = value;
        const bool changed = numberField(label, number, step, NumberKind::Float, format);
        if (changed) {
            value = static_cast<float>(number);
        }
        return changed;
    }

    bool inputU32(std::string_view label, std::uint32_t& value) {
        double number = value;
        const bool changed = numberField(label, number, 0.0, NumberKind::U32, "%u");
        if (changed) {
            value = static_cast<std::uint32_t>(std::clamp<double>(number, 0.0,
                std::numeric_limits<std::uint32_t>::max()));
        }
        return changed;
    }

    bool sliderInt(std::string_view label, int& value, int min, int max, const char* format) {
        auto& container = current();
        const auto key = uniqueKey(container, keyFor(label));
        auto& widget = obtain(container, key, WidgetKind::Slider, [](Widget& w) {
            NSSlider* slider = [NSSlider sliderWithValue:0 minValue:0 maxValue:1 target:nil
                                                  action:@selector(sliderChanged:)];
            slider.target = targetFor(w);
            slider.continuous = YES;
            w.view = slider;
            w.control = slider;
        });
        NSSlider* slider = (NSSlider*)widget.control;
        if (slider.minValue != min || slider.maxValue != max) {
            slider.minValue = min;
            slider.maxValue = max;
            slider.numberOfTickMarks = 0;
        }
        bool changed = false;
        if (widget.changed) {
            widget.changed = false;
            const int newValue = static_cast<int>(std::lround(widget.number));
            if (newValue != value) {
                value = newValue;
                changed = true;
            }
        }
        if (static_cast<int>(std::lround(slider.doubleValue)) != value) {
            slider.doubleValue = value;
        }

        const float width = takeItemWidth(container);
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), format != nullptr ? format : "%d", value);
        const float valueWidth = std::max(28.0F, measureText(currentFont(), currentFontSize(), buffer).x + 6.0F);
        const ItemRect rect = placeItem(container, { width + labelWidth(label), frameHeight() }, true);
        const ItemRect sliderRect = { rect.min, { rect.min.x + std::max(20.0F, width - valueWidth), rect.max.y } };
        placeView(container, widget, sliderRect);

        DrawCmd cmd;
        cmd.type = DrawCmd::Type::Text;
        cmd.coords = { sliderRect.max.x + 4.0F,
                       rect.min.y + (frameHeight() - textLineHeight()) * 0.5F, 0, 0, 0, 0 };
        cmd.color = textColor();
        cmd.font = currentFont();
        cmd.fontSize = currentFontSize();
        cmd.text = buffer;
        appendDrawCmd(container, std::move(cmd));
        drawLabel(container, { rect.min, { rect.min.x + width, rect.max.y } }, label);
        container.lastItemRect.max.x = containerOrigin(container).x + rect.max.x;
        return changed;
    }

    void progressBar(float fraction, Vec2 size, std::string_view overlay) {
        auto& container = current();
        const auto key = uniqueKey(container, hashBytes(seed(), "##progress", 10));
        auto& widget = obtain(container, key, WidgetKind::Progress, [](Widget& w) {
            NSProgressIndicator* bar = [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(0, 0, 100, 20)];
            bar.style = NSProgressIndicatorStyleBar;
            bar.indeterminate = NO;
            bar.minValue = 0.0;
            bar.maxValue = 1.0;
            w.view = bar;
        });
        NSProgressIndicator* bar = (NSProgressIndicator*)widget.view;
        const float avail = availableWidth(container);
        const float width = size.x > 0.0F ? size.x : std::max(1.0F, avail + (size.x < 0.0F ? size.x + 1.0F : 0.0F));
        const float height = size.y > 0.0F ? size.y : frameHeight();
        std::string caption = overlay.empty()
            ? std::to_string(static_cast<int>(std::lround(std::clamp(fraction, 0.0F, 1.0F) * 100.0F))) + "%"
            : std::string(overlay);
        const float captionWidth = measureText(currentFont(), currentFontSize(), caption).x + 8.0F;
        const ItemRect rect = placeItem(container, { width, height }, true);
        const ItemRect barRect = { rect.min, { std::max(rect.min.x + 10.0F, rect.max.x - captionWidth), rect.max.y } };
        placeView(container, widget, barRect);
        if (std::abs(bar.doubleValue - fraction) > 0.0005) {
            bar.doubleValue = std::clamp(fraction, 0.0F, 1.0F);
        }
        DrawCmd cmd;
        cmd.type = DrawCmd::Type::Text;
        cmd.coords = { barRect.max.x + 6.0F, rect.min.y + (height - textLineHeight()) * 0.5F, 0, 0, 0, 0 };
        cmd.color = textColor();
        cmd.font = currentFont();
        cmd.fontSize = currentFontSize();
        cmd.text = caption;
        appendDrawCmd(container, std::move(cmd));
        container.lastItemRect.max.x = containerOrigin(container).x + rect.max.x;
    }

    bool beginCombo(std::string_view label, std::string_view preview, ComboFlags flags) {
        auto& s = state();
        auto& container = current();
        const auto key = uniqueKey(container, keyFor(label));
        auto& widget = obtain(container, key, WidgetKind::Combo, [](Widget& w) {
            NSPopUpButton* popup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(0, 0, 120, 22) pullsDown:YES];
            QPTarget* target = targetFor(w);
            popup.target = target;
            popup.action = @selector(popUpChosen:);
            popup.font = [NSFont systemFontOfSize:13];
            popup.menu.delegate = (id<NSMenuDelegate>)target;
            w.view = popup;
            w.control = popup;
        });
        const bool noPreview = hasFlag(flags, ComboFlags::NoPreview);
        const float width = noPreview ? frameHeight() + 14.0F : takeItemWidth(container);
        const ItemRect rect = placeItem(container, { width + labelWidth(label), frameHeight() }, true);
        const ItemRect controlRect = { rect.min, { rect.min.x + width, rect.max.y } };
        placeView(container, widget, controlRect);
        drawLabel(container, controlRect, label);
        container.lastItemRect.max.x = containerOrigin(container).x + rect.max.x;

        widget.items.clear();
        widget.itemSelected.clear();
        widget.itemCursor = 0;
        widget.shownText = noPreview ? std::string() : std::string(displayPart(preview));
        s.combo = &widget;
        return true;
    }

    void endCombo() {
        auto& s = state();
        Widget* widget = s.combo;
        s.combo = nullptr;
        if (widget == nullptr) {
            return;
        }
        NSPopUpButton* popup = (NSPopUpButton*)widget->control;
        widget->chosenIndex = -1;
        // Not while the menu is open: rebuilding it would close it under the user's mouse.
        if (widget->editing) {
            return;
        }
        std::vector<std::string> all;
        all.reserve(widget->items.size() + 1);
        all.push_back(widget->shownText);
        all.insert(all.end(), widget->items.begin(), widget->items.end());
        if (all != widget->shownItems) {
            [popup removeAllItems];
            for (const auto& item : all) {
                // addItemWithTitle: would merge items with equal titles; this keeps each.
                NSMenuItem* menuItem = [[NSMenuItem alloc] initWithTitle:toNS(item) action:nil keyEquivalent:@""];
                [popup.menu addItem:menuItem];
            }
            widget->shownItems = all;
        }
        for (std::size_t index = 0; index < widget->itemSelected.size(); ++index) {
            NSMenuItem* item = [popup itemAtIndex:static_cast<NSInteger>(index + 1)];
            const NSControlStateValue wanted = widget->itemSelected[index] ? NSControlStateValueOn
                                                                           : NSControlStateValueOff;
            if (item.state != wanted) {
                item.state = wanted;
            }
        }
    }

    bool selectable(std::string_view label, bool selected, SelectableFlags flags, Vec2 size) {
        auto& s = state();
        if (s.combo != nullptr) {
            Widget& widget = *s.combo;
            const int index = widget.itemCursor++;
            widget.items.emplace_back(displayPart(label));
            widget.itemSelected.push_back(selected);
            return widget.chosenIndex == index;
        }
        auto& container = current();
        const auto key = uniqueKey(container, keyFor(label));
        auto& widget = obtain(container, key, WidgetKind::Selectable, [](Widget& w) {
            QPSelectableView* view = [[QPSelectableView alloc] initWithFrame:NSMakeRect(0, 0, 100, 18)];
            view.widget = &w;
            w.view = view;
        });
        QPSelectableView* view = (QPSelectableView*)widget.view;
        view.title = toNS(displayPart(label));
        view.selected = selected;
        const float avail = availableWidth(container);
        const float width = size.x > 0.0F ? size.x : std::max(1.0F, avail);
        const float height = size.y > 0.0F ? size.y : textLineHeight() + 2.0F;
        const ItemRect rect = placeItem(container, { width, height }, false);
        placeView(container, widget, rect);
        const bool clicked = widget.clicked && !isDisabled();
        widget.clicked = false;
        if (clicked && !hasFlag(flags, SelectableFlags::DontClosePopups) && !s.popupScopes.empty() &&
            !s.popupScopes.back().modal) {
            closePopup(s.popupScopes.back().key, false);
        }
        return clicked;
    }

    void setItemDefaultFocus() {
    }

    bool collapsingHeader(std::string_view label, TreeNodeFlags flags) {
        auto& container = current();
        const auto key = uniqueKey(container, keyFor(label));
        auto& widget = obtain(container, key, WidgetKind::Section, [](Widget& w) {
            QPSectionView* view = [[QPSectionView alloc] initWithFrame:NSMakeRect(0, 0, 100, 22)];
            view.widget = &w;
            w.view = view;
        });
        QPSectionView* view = (QPSectionView*)widget.view;
        const bool leaf = hasFlag(flags, TreeNodeFlags::Leaf);
        if (!widget.openInitialized) {
            widget.open = hasFlag(flags, TreeNodeFlags::DefaultOpen);
            widget.openInitialized = true;
        }
        if (widget.clicked) {
            widget.clicked = false;
            if (!leaf) {
                widget.open = !widget.open;
            }
        }
        view.title = toNS(displayPart(label));
        view.open = widget.open;
        view.leaf = leaf;
        view.selected = hasFlag(flags, TreeNodeFlags::Selected);
        const float avail = availableWidth(container);
        const ItemRect rect = placeItem(container, { std::max(1.0F, avail), frameHeight() }, true);
        placeView(container, widget, rect);
        return leaf ? true : widget.open;
    }

    bool iconButton(std::string_view id, std::string_view label, Vec2 size, int state,
        const IconPainter& painter) {
        constexpr int BUTTON_ACTIVE = 1;
        constexpr int BUTTON_HIGHLIGHTED = 2;
        constexpr int BUTTON_DISABLED = 3;

        auto& container = current();
        const Vec2 topLeft = getCursorScreenPos();
        bool clicked = invisibleButton(id, size);
        if (state == BUTTON_DISABLED) {
            clicked = false;
        }
        QaplaUi::DrawList& drawList = container.drawList;

        Color32 background = 0;
        if (state == BUTTON_DISABLED) {
            background = toColor32(Vec4(0.3F, 0.3F, 0.3F, 0.5F));
        } else if (isItemActive() || state == BUTTON_ACTIVE) {
            background = styleColor(StyleColor::ButtonActive);
        } else if (isItemHovered()) {
            background = styleColor(StyleColor::ButtonHovered);
        } else {
            background = styleColor(StyleColor::Button);
        }
        drawList.addRectFilled(topLeft, topLeft + size, background, 5.0F);
        if (painter) {
            painter(drawList, topLeft, size);
        }
        if (state == BUTTON_HIGHLIGHTED) {
            constexpr float dotRadius = 6.0F;
            constexpr float dotLocation = 2.0F;
            drawList.addCircleFilled({ topLeft.x + size.x - dotLocation, topLeft.y + dotLocation },
                dotRadius, col32(192, 0, 0, 192));
        }
        const Vec2 labelSize = calcTextSize(label);
        const Color32 labelColor = state == BUTTON_DISABLED ? styleColor(StyleColor::TextDisabled)
                                                            : styleColor(StyleColor::Text);
        drawList.addText({ topLeft.x + size.x * 0.5F - labelSize.x * 0.5F, topLeft.y + size.y + 4.0F },
            labelColor, label);
        return clicked;
    }

    float splitter(std::string_view id, Vec2 size, bool dragsVertically, bool interactive) {
        auto& container = current();
        const auto key = uniqueKey(container, keyFor(id));
        auto& widget = obtain(container, key, WidgetKind::Splitter, [](Widget& w) {
            QPSplitterView* view = [[QPSplitterView alloc] initWithFrame:NSMakeRect(0, 0, 4, 4)];
            view.widget = &w;
            w.view = view;
        });
        QPSplitterView* view = (QPSplitterView*)widget.view;
        if (view.vertical != dragsVertically || view.interactive != interactive) {
            view.vertical = dragsVertically;
            view.interactive = interactive;
            [view.window invalidateCursorRectsForView:view];
            [view setNeedsDisplay:YES];
        }
        const ItemRect rect = placeItem(container, size, false);
        placeView(container, widget, rect);
        const float delta = interactive ? widget.dragDelta : 0.0F;
        widget.dragDelta = 0.0F;
        container.lastItemActive = widget.dragging;
        return delta;
    }

    // Items

    bool isItemHovered() { return current().lastItemHovered; }
    bool isItemActive() { return current().lastItemActive; }
    bool isItemClicked(MouseButton button) {
        auto& container = current();
        return container.lastItemHovered && clickedIn(container, static_cast<std::size_t>(button));
    }
    bool isItemVisible() { return true; }
    bool isItemDeactivatedAfterEdit() { return current().lastItemDeactivated; }
    Vec2 getItemRectMin() { return current().lastItemRect.min; }
    Vec2 getItemRectMax() { return current().lastItemRect.max; }

    void setTooltip(std::string_view text) { itemTooltip(text); }

    void itemTooltip(std::string_view text) {
        if (text.empty()) {
            return;
        }
        auto& container = current();
        if (container.lastItemWidget != nullptr) {
            Widget& widget = *container.lastItemWidget;
            if (widget.shownTooltip != text) {
                widget.shownTooltip = std::string(text);
                widget.view.toolTip = toNS(text);
            }
            return;
        }
        const Vec2 origin = containerOrigin(container);
        const ItemRect& rect = container.lastItemRect;
        container.tooltips.push_back({ NSMakeRect(rect.min.x - origin.x, rect.min.y - origin.y,
                                                  rect.max.x - rect.min.x, rect.max.y - rect.min.y),
                                       std::string(text) });
    }

    // Containers

    bool beginChild(std::string_view id, Vec2 size, ChildFlags childFlags, WindowFlags windowFlags) {
        auto& parent = current();
        const auto key = uniqueKey(parent, keyFor(id));
        auto& slot = parent.children[key];
        const bool horizontal = hasFlag(windowFlags, WindowFlags::HorizontalScrollbar) ||
            hasFlag(windowFlags, WindowFlags::AlwaysHorizontalScrollbar);
        if (!slot) {
            slot = std::make_unique<Container>();
            slot->kind = ContainerKind::Child;
            slot->key = key;
            slot->view = makeContainerView();
            makeScrollable(*slot, horizontal);
            [parent.view addSubview:slot->outer];
        }
        auto& child = *slot;
        child.window = parent.window;
        child.horizontalScroll = horizontal;
        child.noScrollbar = hasFlag(windowFlags, WindowFlags::NoScrollbar);
        child.hasBorder = hasFlag(childFlags, ChildFlags::Border);
        auto* scroll = (QPScrollView*)child.scroll;
        scroll.hasVerticalScroller = !child.noScrollbar;
        scroll.ignoresScrollWheel = hasFlag(windowFlags, WindowFlags::NoScrollWithMouse);
        scroll.borderType = child.hasBorder ? NSLineBorder : NSNoBorder;

        const Vec2 avail = getContentRegionAvail();
        Vec2 actual = size;
        if (hasFlag(childFlags, ChildFlags::AutoResizeY) && child.lastContentSize.y > 0.0F) {
            actual.y = child.lastContentSize.y;
        } else if (actual.y <= 0.0F) {
            actual.y = std::max(4.0F, avail.y + actual.y);
        }
        if (hasFlag(childFlags, ChildFlags::AutoResizeX) && child.lastContentSize.x > 0.0F) {
            actual.x = child.lastContentSize.x;
        } else if (actual.x <= 0.0F) {
            actual.x = std::max(4.0F, avail.x + actual.x);
        }

        const ItemRect rect = placeItem(parent, actual, false);
        const NSRect frame = toNSRect(rect);
        if (!NSEqualRects(child.outer.frame, frame)) {
            child.outer.frame = frame;
        }
        parent.elements.push_back({ child.outer, -1 });

        child.size = actual;
        child.padding = child.hasBorder ? state().style.windowPadding : Vec2();
        child.scrollY = static_cast<float>(child.scroll.contentView.bounds.origin.y);
        child.originInWindow = containerOrigin(parent) + rect.min - Vec2(0.0F, child.scrollY);

        state().idStack.push_back(key);
        pushContainer(child);
        child.lastItemHovered = false;
        return true;
    }

    void endChild() {
        auto& s = state();
        if (s.stack.size() <= 1) {
            return;
        }
        Container* child = s.stack.back();
        popContainer();
        popId();
        // The child as an item of its parent, for the isItem...() queries that follow.
        auto& parent = current();
        const Vec2 origin = containerOrigin(parent);
        const NSRect frame = child->outer.frame;
        parent.lastItemRect = { origin + Vec2(static_cast<float>(frame.origin.x), static_cast<float>(frame.origin.y)),
                                origin + Vec2(static_cast<float>(NSMaxX(frame)), static_cast<float>(NSMaxY(frame))) };
        parent.lastItemWidget = nullptr;
    }

    void beginOverlay(std::string_view id, Vec2 position, Vec2 size, WindowFlags flags) {
        static_cast<void>(flags);
        auto& s = state();
        const auto key = hashBytes(FNV_OFFSET, id.data(), id.size());
        auto& container = beginTopLevel(s.overlays, key, ContainerKind::Overlay);
        container.window = s.window;
        if (container.outer.superview != s.root) {
            [s.root addSubview:container.outer];
        }
        const NSRect frame = NSMakeRect(position.x, position.y, size.x, size.y);
        if (!NSEqualRects(container.outer.frame, frame)) {
            container.outer.frame = frame;
        }
        container.size = size;
        container.originInWindow = position;
        container.padding = s.style.windowPadding;
        const Color32 background = styleColor(StyleColor::WindowBg);
        if ((background >> 24U) != 0) {
            container.background = background;
        } else {
            container.background.reset();
        }
        container.rounding = windowRounding();
        s.overlayOrder.push_back(&container);
        s.idStack.push_back(key);
        pushContainer(container);
    }

    void endOverlay() {
        auto& s = state();
        if (s.stack.size() <= 1) {
            return;
        }
        popContainer();
        popId();
    }

    float getScrollY() { return current().scrollY; }
    void setScrollY(float scrollY) { current().requestedScrollY = scrollY; }
    float getScrollMaxX() {
        auto& container = current();
        return container.horizontalScroll ? std::max(0.0F, container.lastContentSize.x - container.size.x) : 0.0F;
    }
    float getScrollMaxY() {
        auto& container = current();
        return std::max(0.0F, container.lastContentSize.y - container.size.y);
    }
    void setScrollHereY(float centerYRatio) {
        auto& container = current();
        container.requestedScrollRatio = cursorPos(container).y - container.size.y * centerYRatio;
    }
    float getWindowWidth() { return current().size.x; }
    float getWindowHeight() { return current().size.y; }
    bool isWindowFocused() {
        auto& container = current();
        NSWindow* window = container.window != nil ? container.window : state().window;
        return window.isKeyWindow;
    }

    // Popups

    void openPopup(std::string_view id) {
        auto& s = state();
        auto open = [&s](std::uint64_t key) {
            auto& popup = s.popups[key];
            if (!popup.open) {
                popup.open = true;
                popup.position = s.mouse;
                popup.openedFrame = s.frame;
                popup.menuShown = false;
                popup.menuChoice.reset();
            }
        };
        open(keyFor(id));
        // Dialogs are known by their title alone; see beginPopupModal().
        open(hashBytes(FNV_OFFSET, id.data(), id.size()));
        scheduleFrame();
    }

    bool beginPopup(std::string_view id) {
        auto& s = state();
        const auto key = keyFor(id);
        auto found = s.popups.find(key);
        if (found == s.popups.end() || !found->second.open) {
            return false;
        }
        auto& container = beginTopLevel(s.popupContainers, key, ContainerKind::Popup);
        container.window = s.window;
        if (container.outer.superview != s.root) {
            [s.root addSubview:container.outer];
            container.view.wantsLayer = YES;
            container.view.shadow = [[NSShadow alloc] init];
            container.view.layer.shadowOpacity = 0.4F;
            container.view.layer.shadowRadius = 8.0;
        }
        Vec2 size = container.lastContentSize;
        if (size.x <= 0.0F || size.y <= 0.0F) {
            size = { 40.0F, 20.0F };
        }
        const Vec2 viewport = getViewportSize();
        Vec2 position = found->second.position;
        position.x = std::clamp(position.x, 0.0F, std::max(0.0F, viewport.x - size.x));
        position.y = std::clamp(position.y, 0.0F, std::max(0.0F, viewport.y - size.y));
        const NSRect frame = NSMakeRect(position.x, position.y, size.x, size.y);
        if (!NSEqualRects(container.outer.frame, frame)) {
            container.outer.frame = frame;
        }
        container.size = size;
        container.originInWindow = position;
        container.padding = s.style.windowPadding;
        container.background = styleColor(StyleColor::PopupBg) | 0xFF000000U;
        container.rounding = 6.0F;
        s.popupOrder.push_back(&container);
        s.popupScopes.push_back({ key, false });
        s.idStack.push_back(key);
        pushContainer(container);
        return true;
    }

    bool beginPopupModal(std::string_view title, bool* open, Vec2 initialSize) {
        static_cast<void>(open);
        auto& s = state();
        const auto key = hashBytes(FNV_OFFSET, title.data(), title.size());
        auto found = s.popups.find(key);
        if (found == s.popups.end() || !found->second.open) {
            return false;
        }
        auto& container = beginTopLevel(s.modals, key, ContainerKind::Modal);
        if (container.panel == nil) {
            const NSSize size = NSMakeSize(initialSize.x > 0.0F ? initialSize.x : 480.0F,
                                           initialSize.y > 0.0F ? initialSize.y : 360.0F);
            NSPanel* panel = [[NSPanel alloc] initWithContentRect:NSMakeRect(0, 0, size.width, size.height)
                styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskResizable
                backing:NSBackingStoreBuffered defer:NO];
            panel.title = toNS(displayPart(title));
            panel.releasedWhenClosed = NO;
            panel.contentView = container.view;
            container.outer = container.view;
            container.panel = panel;
            container.window = panel;
            [s.window beginSheet:panel completionHandler:nil];
        }
        const NSSize contentSize = container.panel.contentView.bounds.size;
        container.size = { static_cast<float>(contentSize.width), static_cast<float>(contentSize.height) };
        container.originInWindow = {};
        container.padding = s.style.windowPadding;
        s.popupScopes.push_back({ key, true });
        s.idStack.push_back(key);
        pushContainer(container);
        return true;
    }

    void endPopup() {
        auto& s = state();
        if (s.stack.size() <= 1) {
            return;
        }
        popContainer();
        popId();
        if (!s.popupScopes.empty()) {
            s.popupScopes.pop_back();
        }
    }

    void closeCurrentPopup() {
        auto& s = state();
        if (s.popupScopes.empty()) {
            return;
        }
        const auto scope = s.popupScopes.back();
        closePopup(scope.key, scope.modal);
    }

    std::optional<std::size_t> popupMenu(std::string_view id, const std::vector<MenuItem>& items) {
        auto& s = state();
        const auto key = keyFor(id);
        auto found = s.popups.find(key);
        if (found == s.popups.end()) {
            return std::nullopt;
        }
        auto& popup = found->second;
        if (popup.menuChoice) {
            auto choice = popup.menuChoice;
            popup.menuChoice.reset();
            return choice;
        }
        if (!popup.open || popup.menuShown) {
            return std::nullopt;
        }
        popup.menuShown = true;

        // Shown after the frame: the menu runs a loop of its own until it closes, and the frame
        // must not wait inside it.
        NSMenu* menu = [[NSMenu alloc] initWithTitle:@""];
        menu.autoenablesItems = NO;
        for (const auto& item : items) {
            NSString* title = toNS(item.label);
            NSMenuItem* menuItem = [[NSMenuItem alloc] initWithTitle:title action:nil keyEquivalent:@""];
            menuItem.enabled = item.enabled;
            if (item.highlighted) {
                NSImage* dot = [NSImage imageWithSize:NSMakeSize(8, 8) flipped:NO
                    drawingHandler:^BOOL(NSRect rect) {
                        [[NSColor colorWithSRGBRed:0.75 green:0 blue:0 alpha:0.9] setFill];
                        [[NSBezierPath bezierPathWithOvalInRect:rect] fill];
                        return YES;
                    }];
                menuItem.image = dot;
            }
            [menu addItem:menuItem];
        }
        NSView* root = s.root;
        const NSPoint location = NSMakePoint(popup.position.x, popup.position.y);
        const std::uint64_t popupKey = key;
        dispatch_async(dispatch_get_main_queue(), ^{
            const BOOL chosen = [menu popUpMenuPositioningItem:nil atLocation:location inView:root];
            auto& popups = state().popups;
            auto entry = popups.find(popupKey);
            if (entry != popups.end()) {
                entry->second.open = false;
                if (chosen && menu.highlightedItem != nil) {
                    entry->second.menuChoice = static_cast<std::size_t>([menu indexOfItem:menu.highlightedItem]);
                }
            }
            scheduleFrame();
        });
        return std::nullopt;
    }

    // Tabs

    bool beginTabBar(std::string_view id, TabBarFlags flags) {
        static_cast<void>(flags);
        auto& container = current();
        const auto key = uniqueKey(container, keyFor(id));
        auto& widget = obtain(container, key, WidgetKind::TabStrip, [](Widget& w) {
            QPTabStripView* view = [[QPTabStripView alloc] initWithFrame:NSMakeRect(0, 0, 100, 28)];
            view.widget = &w;
            w.view = view;
        });
        const float avail = availableWidth(container);
        const ItemRect rect = placeItem(container, { std::max(1.0F, avail), 28.0F }, true);
        placeView(container, widget, rect);
        widget.tabs.clear();
        widget.hasAddButton = false;
        widget.tabOwner = &container;
        state().tabBars.push_back(&widget);
        return true;
    }

    void endTabBar() {
        auto& s = state();
        if (s.tabBars.empty()) {
            return;
        }
        Widget& widget = *s.tabBars.back();
        s.tabBars.pop_back();
        const bool selectedPresent = std::ranges::any_of(widget.tabs,
            [&widget](const TabInfo& tab) { return tab.key == widget.selectedTab; });
        if (!selectedPresent && !widget.tabs.empty()) {
            widget.selectedTab = widget.tabs.front().key;
            scheduleFrame();
        }
        widget.clickedTab = 0;
        widget.closedTab = 0;
        if (widget.tabs != widget.shownTabs || widget.selectedTab != widget.shownTabSelected ||
            widget.hasAddButton != widget.shownState) {
            [(QPTabStripView*)widget.view setTabs:widget.tabs selected:widget.selectedTab
                                        addButton:widget.hasAddButton];
            widget.shownTabs = widget.tabs;
            widget.shownTabSelected = widget.selectedTab;
            widget.shownState = widget.hasAddButton;
        }
    }

    bool beginTabItem(std::string_view label, bool* open, TabItemFlags flags, bool highlighted,
        std::string_view tooltip) {
        auto& s = state();
        if (s.tabBars.empty()) {
            return false;
        }
        Widget& widget = *s.tabBars.back();
        auto part = idPart(label);
        const auto key = hashBytes(widget.key, part.data(), part.size());
        if (widget.closedTab == key) {
            widget.closedTab = 0;
            if (open != nullptr) {
                *open = false;
            }
            scheduleFrame();
        }
        if (widget.clickedTab == key) {
            widget.clickedTab = 0;
            widget.selectedTab = key;
        }
        if (hasFlag(flags, TabItemFlags::SetSelected) && widget.selectedTab != key) {
            widget.selectedTab = key;
            scheduleFrame();
        }
        if (widget.selectedTab == 0) {
            widget.selectedTab = key;
        }
        widget.tabs.push_back({ key, std::string(displayPart(label)), std::string(tooltip),
                                open != nullptr, highlighted });
        if (widget.selectedTab != key) {
            return false;
        }
        s.idStack.push_back(key);
        return true;
    }

    void endTabItem() { popId(); }

    bool tabItemButton(std::string_view label, TabItemFlags flags) {
        static_cast<void>(label);
        static_cast<void>(flags);
        auto& s = state();
        if (s.tabBars.empty()) {
            return false;
        }
        Widget& widget = *s.tabBars.back();
        widget.hasAddButton = true;
        const bool clicked = widget.addClicked;
        widget.addClicked = false;
        return clicked;
    }

    // Tables

    std::optional<std::size_t> table(QaplaWindows::ImGuiTable& table, Vec2 size) {
        auto& container = current();
        const auto& id = table.tableId();
        const auto key = uniqueKey(container, hashBytes(seed(), id.data(), id.size()));
        auto& widget = obtain(container, key, WidgetKind::Table, [](Widget& w) {
            w.view = makeTableView(w);
        });
        const Vec2 avail = getContentRegionAvail();
        const float width = size.x > 0.0F ? std::min(size.x, std::max(avail.x, 40.0F)) : std::max(40.0F, avail.x);
        const float rowHeight = lineHeight(table.font(), baseFontSize(table.font())) + 4.0F;
        const float contentHeight = static_cast<float>(table.visibleRowCount() + 1) * rowHeight + 30.0F;
        float height = 0.0F;
        if (hasFlag(table.tableFlags(), TableFlags::ScrollY)) {
            height = size.y > 0.0F ? std::min(size.y, std::max(contentHeight, rowHeight * 3.0F))
                                   : std::max(rowHeight * 3.0F, avail.y);
        } else {
            height = contentHeight;
        }
        const ItemRect rect = placeItem(container, { width, height }, false);
        placeView(container, widget, rect);
        updateTableView(widget, table, toNSRect(rect));
        auto picked = widget.pickedRow;
        widget.pickedRow.reset();
        return picked;
    }

    // Input

    bool isKeyPressed(Key key, bool repeat) {
        if (editingText()) {
            return false;
        }
        return std::ranges::any_of(state().pressed,
            [key, repeat](const Pressed& pressed) { return pressed.key == key && (repeat || !pressed.repeat); });
    }

    bool isKeyDown(Key key) {
        const auto modifiers = state().modifiers;
        switch (key) {
        case Key::LeftCtrl:
        case Key::RightCtrl: return (modifiers & NSEventModifierFlagControl) != 0;
        case Key::LeftShift:
        case Key::RightShift: return (modifiers & NSEventModifierFlagShift) != 0;
        case Key::LeftAlt:
        case Key::RightAlt: return (modifiers & NSEventModifierFlagOption) != 0;
        case Key::LeftSuper:
        case Key::RightSuper: return (modifiers & NSEventModifierFlagCommand) != 0;
        default:
            break;
        }
        if (editingText()) {
            return false;
        }
        return state().keysDown.contains(static_cast<int>(key));
    }

    bool isShortcutModifierDown() {
        return (state().modifiers & NSEventModifierFlagCommand) != 0;
    }

    bool isMouseClicked(MouseButton button) { return state().clicked[static_cast<std::size_t>(button)]; }
    bool isMouseDown(MouseButton button) { return state().down[static_cast<std::size_t>(button)]; }
    bool isMouseReleased(MouseButton button) { return state().released[static_cast<std::size_t>(button)]; }
    Vec2 getMousePos() {
        return state().stack.empty() ? state().mouse : mouseFor(current());
    }
    Vec2 getMouseDelta() { return state().mouse - state().previousMouse; }
    void setMouseCursor(MouseCursor cursor) {
        // Cursors on the Mac belong to the views under the mouse, which set their own.
        static_cast<void>(cursor);
    }
    bool wantsTextInput() { return editingText(); }

    double getTime() { return state().frameTime - state().startTime; }
    int getFrameCount() { return state().frame; }
    float getDeltaTime() { return state().deltaTime; }

    std::string getClipboardText() {
        NSString* text = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
        return text != nil ? fromNS(text) : std::string();
    }

    void setClipboardText(std::string_view text) {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard clearContents];
        [pasteboard setString:toNS(text) forType:NSPasteboardTypeString];
    }

    void requestFrame() {
        state().fastUntil = std::max(state().fastUntil, CACurrentMediaTime() + 0.1);
    }

    std::string_view frontendName() { return "macos"; }

} // namespace QaplaUi
