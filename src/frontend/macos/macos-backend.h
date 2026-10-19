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

#pragma once

/**
 * @file macos-backend.h
 * @brief The inner workings of QaplaUi on AppKit, shared by the files of the macOS frontend.
 *
 * QaplaUi describes the interface anew in every frame; AppKit keeps its views. The bridge is a
 * tree of Containers, one per window, child region, popup or overlay. Each Container keeps the
 * Widgets it made, keyed by the identifier of the call that asked for them. A call in the next
 * frame with the same identifier gets the same widget back: its view is moved and updated only
 * where something changed, and what the user did to it since -- a click, typed text -- is what
 * the call returns. A widget no call asked for in a frame is removed at its end.
 *
 * Drawing (the board, the clocks, the icons) is recorded as DrawCmds and painted by layer views
 * that sit between the controls in the order the calls came in, so what was drawn after a control
 * is above it, as with any immediate-mode toolkit.
 */

#import <AppKit/AppKit.h>

#include "ui/qapla-ui.h"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

@class QPContainerView;
@class QPLayerView;

namespace QaplaMac {

    using QaplaUi::Vec2;
    using QaplaUi::Vec4;
    using QaplaUi::Color32;

    // ------------------------------------------------------------------------------------------
    // Drawing
    // ------------------------------------------------------------------------------------------

    struct DrawCmd {
        enum class Type : std::uint8_t {
            Line, Rect, RectFilled, Circle, CircleFilled, Triangle, Text, Polyline, PushClip, PopClip
        };
        Type type = Type::Line;
        std::array<float, 6> coords{};
        Color32 color = 0;
        float thickness = 1.0F;
        float rounding = 0.0F;
        bool closed = false;
        QaplaUi::Font font = QaplaUi::Font::Inter;
        float fontSize = 0.0F;
        float wrapWidth = 0.0F;
        std::string text;
        std::vector<Vec2> points;

        bool operator==(const DrawCmd& other) const = default;
    };

    struct Container;

    /** @brief Adds a command to a container's drawing, after whatever it holds so far. */
    void appendDrawCmd(Container& container, DrawCmd&& cmd);

    /** @brief Where a container's (0,0) is in its window, at this moment of the frame. */
    Vec2 containerOrigin(const Container& container);

    /**
     * @brief A container's DrawList: records in its coordinates, in the order of the calls.
     *
     * Bound to the container rather than to a buffer: a caller may keep the list while it
     * describes further controls, and what it draws after one of them has to land above it.
     */
    class RecordingDrawList : public QaplaUi::DrawList {
    public:
        RecordingDrawList() = default;
        void bind(Container* container) {
            container_ = container;
            path_.clear();
        }

        void addLine(Vec2 from, Vec2 to, Color32 color, float thickness) override;
        void addRect(Vec2 min, Vec2 max, Color32 color, float rounding, float thickness) override;
        void addRectFilled(Vec2 min, Vec2 max, Color32 color, float rounding) override;
        void addCircle(Vec2 center, float radius, Color32 color, int segments, float thickness) override;
        void addCircleFilled(Vec2 center, float radius, Color32 color, int segments) override;
        void addTriangleFilled(Vec2 first, Vec2 second, Vec2 third, Color32 color) override;
        void addText(Vec2 position, Color32 color, std::string_view text) override;
        void addText(QaplaUi::Font font, float size, Vec2 position, Color32 color,
            std::string_view text) override;
        void pathArcTo(Vec2 center, float radius, float fromAngle, float toAngle, int segments) override;
        void pathStroke(Color32 color, bool closed, float thickness) override;
        void pushClipRect(Vec2 min, Vec2 max, bool intersectWithCurrent) override;
        void popClipRect() override;

        /** @brief Text with explicit wrapping, for the text items. */
        void addWrappedText(QaplaUi::Font font, float size, Vec2 position, Color32 color,
            std::string_view text, float wrapWidth);

    private:
        Vec2 local(Vec2 point) const;
        void push(DrawCmd&& cmd);
        Container* container_ = nullptr;
        std::vector<Vec2> path_;
    };

    /** @brief Paints recorded commands into the current graphics context; y grows downwards. */
    void paint(const std::vector<DrawCmd>& commands, CGContextRef context, float alpha = 1.0F);

    NSColor* nsColor(Color32 color);
    NSColor* nsColor(const Vec4& color);

    // ------------------------------------------------------------------------------------------
    // Fonts
    // ------------------------------------------------------------------------------------------

    /** @brief Registers the fonts embedded in the application; once, before the first frame. */
    void registerFonts();

    /** @brief The size a font has unless something scaled it. */
    float baseFontSize(QaplaUi::Font font);

    NSFont* nsFont(QaplaUi::Font font, float size);

    /** @brief Size of a text set in a font; wrapped if wrapWidth is positive. */
    Vec2 measureText(QaplaUi::Font font, float size, std::string_view text, float wrapWidth = -1.0F);

    /** @brief Height of a line of text in a font, the step from one line to the next. */
    float lineHeight(QaplaUi::Font font, float size);

    // ------------------------------------------------------------------------------------------
    // Widgets and containers
    // ------------------------------------------------------------------------------------------

    enum class WidgetKind : std::uint8_t {
        Button, Checkbox, Radio, TextField, MultilineText, NumberField, Slider, Progress, Combo,
        Section, Selectable, TabStrip, Table, Splitter, IconButton, Child
    };

    struct ItemRect {
        Vec2 min;
        Vec2 max;
        [[nodiscard]] bool contains(Vec2 point) const {
            return point.x >= min.x && point.y >= min.y && point.x < max.x && point.y < max.y;
        }
        [[nodiscard]] Vec2 size() const { return max - min; }
    };

    /** @brief One tab as it was declared in this frame. */
    struct TabInfo {
        std::uint64_t key = 0;
        std::string title;
        std::string tooltip;
        bool closable = false;
        bool highlighted = false;
        bool operator==(const TabInfo& other) const = default;
    };

    /**
     * @brief A control made for one call, kept from frame to frame.
     *
     * The fields from "Events" on are written by AppKit's callbacks between frames and read --
     * and cleared -- by the call of the next frame.
     */
    struct Widget {
        WidgetKind kind = WidgetKind::Button;
        std::uint64_t key = 0;
        int lastFrame = -1;

        NSView* view = nil;        ///< What is placed in the container
        NSControl* control = nil;  ///< The control the user works with, if not the view itself
        NSView* extra = nil;       ///< A second view: the stepper of a number field
        id target = nil;           ///< The object receiving the control's actions

        // What was last shown, so AppKit is only touched on a change
        NSRect shownFrame = NSZeroRect;
        std::string shownTitle;
        std::string shownText;
        std::string shownTooltip;
        double shownNumber = 0.0;
        bool shownEnabled = true;
        bool shownState = false;
        std::vector<std::string> shownItems;
        std::vector<TabInfo> shownTabs;
        std::uint64_t shownTabSelected = 0;
        std::uint64_t shownVersion = 0;
        Vec2 naturalSize;             ///< Size the control wants for its title

        // Events
        bool clicked = false;
        bool changed = false;
        bool enterPressed = false;
        bool deactivated = false;
        bool editing = false;
        std::string text;
        double number = 0.0;
        int chosenIndex = -1;
        std::uint64_t clickedTab = 0;
        std::uint64_t closedTab = 0;
        bool addClicked = false;
        float dragDelta = 0.0F;
        bool dragging = false;
        std::optional<std::size_t> pickedRow;
        std::optional<std::pair<std::size_t, bool>> sortRequest;
        bool stepped = false;         ///< A number field's stepper was clicked

        // State kept by the frontend
        bool open = false;
        bool openInitialized = false;
        bool submitOnEnter = false;   ///< Multiline text: Enter submits, Ctrl+Enter breaks the line
        bool hovered = false;         ///< Kept by views that track the mouse themselves
        std::uint64_t selectedTab = 0;

        // Collected during a frame: the items of a combo, the tabs of a tab bar
        std::vector<std::string> items;
        std::vector<bool> itemSelected;
        int itemCursor = 0;
        std::vector<TabInfo> tabs;
        bool hasAddButton = false;
        Container* tabOwner = nullptr;

        // The table a table view shows, and the data source that feeds it
        id dataSource = nil;
    };

    /** @brief A run of drawing between two views, painted by one layer view. */
    struct DrawRun {
        std::vector<DrawCmd> commands;
    };

    /** @brief One entry of a container's z-order: a view, or a run of drawing. */
    struct Element {
        NSView* view = nil;
        int run = -1;
    };

    struct ToolTip {
        NSRect rect;
        std::string text;
        bool operator==(const ToolTip& other) const {
            return NSEqualRects(rect, other.rect) && text == other.text;
        }
    };

    enum class ContainerKind : std::uint8_t { Overlay, Child, Popup, Modal };

    /**
     * @brief A region that lays out its items top to bottom and owns the views it made.
     */
    struct Container {
        ContainerKind kind = ContainerKind::Child;
        std::uint64_t key = 0;
        std::string name;
        int lastFrame = -1;

        QPContainerView* view = nil;   ///< Where items are placed; flipped, scrolled if scroll != nil
        NSScrollView* scroll = nil;
        NSView* outer = nil;           ///< What is placed in the parent: the scroll view or view
        NSPanel* panel = nil;          ///< Modal dialogs: the sheet

        // Geometry, in the container's own coordinates unless named otherwise
        Vec2 size;                     ///< Visible size
        Vec2 padding;
        Vec2 originInWindow;           ///< Where the container's (0,0) is in the window
        float scrollY = 0.0F;
        std::optional<float> requestedScrollY;
        std::optional<float> requestedScrollRatio;
        bool autoResizeY = false;
        bool autoResizeX = false;
        bool hasBorder = false;
        bool noScrollbar = false;
        bool horizontalScroll = false;

        NSWindow* window = nil;        ///< The window it is in: the main one, or a sheet

        // Layout of this frame
        Vec2 cursor;                   ///< Where an item goes that starts a new line
        float indentX = 0.0F;
        float lineY = 0.0F;            ///< Top of the line the last item is on
        float lineHeight = 0.0F;       ///< Height of that line so far
        bool lineHasFrame = false;     ///< Whether a framed control is on it: text then centres
        Vec2 previousItemEnd;          ///< Right end of the last item, on its line
        bool sameLineRequested = false;
        Vec2 sameLinePos;
        Vec2 contentMax;
        Vec2 lastContentSize;          ///< Of the previous frame, for auto-resizing children
        std::optional<float> nextItemWidth;
        std::vector<float> itemWidthStack;
        struct Group {
            Vec2 start;
            float indentX;
            Vec2 contentMax;
        };
        std::vector<Group> groups;

        // Appearance
        std::optional<Color32> background;
        float rounding = 0.0F;
        float fontScale = 1.0F;

        // Z-order and drawing
        std::vector<Element> elements;
        std::vector<DrawRun> runs;
        std::vector<QPLayerView*> layers;
        std::vector<ToolTip> tooltips;
        std::vector<ToolTip> shownTooltips;
        std::vector<NSString*> tooltipStrings;

        // The last item described, for the isItem...() queries
        ItemRect lastItemRect;            ///< In window coordinates
        std::uint64_t lastItemKey = 0;
        Widget* lastItemWidget = nullptr;
        bool lastItemHovered = false;
        bool lastItemDeactivated = false;
        bool lastItemActive = false;

        RecordingDrawList drawList;

        std::unordered_map<std::uint64_t, std::unique_ptr<Widget>> widgets;
        std::unordered_map<std::uint64_t, std::unique_ptr<Container>> children;
        std::unordered_map<std::uint64_t, int> keyUses;   ///< Same key twice in a frame
    };

    // ------------------------------------------------------------------------------------------
    // The frontend's frame
    // ------------------------------------------------------------------------------------------

    /**
     * @brief Called by the main loop around the application's description of a frame.
     */
    void beginFrame(NSWindow* window);
    void endFrame();

    /** @brief Whether something asked for frames to come quickly: input, an animation. */
    bool wantsFastFrames();

    /** @brief Records an input event for the next frame; installed as an event monitor. */
    void recordEvent(NSEvent* event);

    /** @brief The first container laid into the window, for a screenshot. */
    NSView* rootView();

    /** @brief Asks for a frame now, from an AppKit callback. */
    void scheduleFrame();

    /** @brief Installed by the main loop: runs one frame when called. */
    void setFrameScheduler(void (*scheduler)());

} // namespace QaplaMac

// ----------------------------------------------------------------------------------------------
// Views
// ----------------------------------------------------------------------------------------------

/** @brief The view items are placed in: flipped, so y grows downwards as QaplaUi's does. */
@interface QPContainerView : NSView
@property (nonatomic) BOOL hasCustomBackground;
@property (nonatomic, strong) NSColor* customBackground;
@property (nonatomic) CGFloat cornerRadius;
@end

/** @brief Paints one run of recorded drawing; transparent to the mouse. */
@interface QPLayerView : NSView
- (void)setCommands:(const std::vector<QaplaMac::DrawCmd>&)commands;
@end
