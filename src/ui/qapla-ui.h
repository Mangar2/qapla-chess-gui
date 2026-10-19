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
 * @file qapla-ui.h
 * @brief The user interface toolkit the application is written against.
 *
 * Every window, panel and chatbot step draws itself through the functions declared here and never
 * through a GUI library directly. A frontend implements them: src/frontend/imgui with Dear ImGui,
 * src/frontend/macos with AppKit. A further one -- Win32, say -- implements the same declarations
 * and nothing else has to change. See docs/code-doku/GUI_FRONTENDS.md.
 *
 * The model is immediate mode, the one the application was built on: each frame the whole
 * interface is described anew from the application's data, and a function that stands for a
 * control returns what the user did to it since the last frame. A retained toolkit like AppKit
 * keeps its controls between frames and matches each call to the control it made for it the frame
 * before, by its identifier.
 *
 * Identifiers follow the convention every call site already uses: "Label" is shown and identifies
 * the control, "Label##suffix" shows "Label" and is identified by the whole string, "Label###id"
 * shows "Label" and is identified by "id" alone. pushId()/popId() scope identifiers the same way.
 *
 * Coordinates are in pixels with the origin in the top left corner of the application's window,
 * y growing downwards. A frontend may scroll and nest its containers as it likes, as long as
 * positions handed out (cursor, item rectangles, mouse) and positions handed in (cursor, drawing)
 * agree with each other within a frame.
 *
 * Every function is to be called from the UI thread only, between the frontend's frame begin and
 * frame end.
 */

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace QaplaWindows {
    class ImGuiTable;
}

namespace QaplaUi {

    // ------------------------------------------------------------------------------------------
    // Basic types
    // ------------------------------------------------------------------------------------------

    struct Vec2 {
        float x = 0.0F;
        float y = 0.0F;
        constexpr Vec2() = default;
        constexpr Vec2(float xValue, float yValue) : x(xValue), y(yValue) {}
        constexpr Vec2 operator+(const Vec2& other) const { return { x + other.x, y + other.y }; }
        constexpr Vec2 operator-(const Vec2& other) const { return { x - other.x, y - other.y }; }
        constexpr Vec2 operator*(float factor) const { return { x * factor, y * factor }; }
        constexpr Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
        constexpr Vec2& operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }
        constexpr bool operator==(const Vec2& other) const = default;
    };

    /** @brief A color with float components from 0 to 1: red, green, blue, alpha. */
    struct Vec4 {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
        float w = 0.0F;
        constexpr Vec4() = default;
        constexpr Vec4(float red, float green, float blue, float alpha)
            : x(red), y(green), z(blue), w(alpha) {}
        constexpr bool operator==(const Vec4& other) const = default;
    };

    /** @brief A packed color, 8 bits per channel: red in the lowest byte, alpha in the highest. */
    using Color32 = std::uint32_t;

    constexpr Color32 col32(std::uint32_t red, std::uint32_t green, std::uint32_t blue,
        std::uint32_t alpha = 255) {
        return (alpha << 24U) | (blue << 16U) | (green << 8U) | red;
    }
    constexpr Color32 COL32_WHITE = col32(255, 255, 255, 255);
    constexpr Color32 COL32_BLACK = col32(0, 0, 0, 255);
    constexpr Color32 COL32_TRANSPARENT = col32(0, 0, 0, 0);

    Color32 toColor32(const Vec4& color);
    Vec4 toVec4(Color32 color);

    /**
     * @brief The fonts the application draws with.
     *
     * The numbers are the indices persisted in the configuration and used by tables; do not
     * renumber them.
     */
    enum class Font : int {
        System = 0,   ///< The frontend's plain default font
        Chess = 1,    ///< The chess piece font, the glyphs FontManager::pieceSymbol() names
        Inter = 2,    ///< The proportional interface font, the default for text
        Mono = 3      ///< The monospaced font, used for engine output and result tables
    };

    /** @brief Colors of the frontend's theme, for drawing that has to match its controls. */
    enum class StyleColor {
        Text,
        TextDisabled,
        WindowBg,
        PopupBg,
        Border,
        BorderShadow,
        FrameBg,
        Button,
        ButtonHovered,
        ButtonActive,
        Header,
        HeaderHovered,
        Separator,
        TableRowBg,
        TabDimmedSelected
    };

    enum class StyleVar {
        WindowPadding,    ///< Vec2
        WindowRounding,   ///< float
        WindowBorderSize, ///< float
        FramePadding,     ///< Vec2
        ItemSpacing       ///< Vec2
    };

    /** @brief The metrics of the frontend's theme. */
    struct Style {
        Vec2 windowPadding;
        Vec2 framePadding;
        Vec2 itemSpacing;
        float frameRounding = 0.0F;
        float scrollbarSize = 0.0F;
    };

    enum class Key {
        UpArrow, DownArrow, LeftArrow, RightArrow, PageUp, PageDown, Home, End,
        Enter, Escape, Delete, Backspace, Tab,
        A, C, V, X, Z,
        LeftCtrl, RightCtrl, LeftShift, RightShift, LeftAlt, RightAlt, LeftSuper, RightSuper
    };

    enum class MouseButton { Left = 0, Right = 1, Middle = 2 };

    enum class MouseCursor { Arrow, TextInput, ResizeEW, ResizeNS, Hand };

    // Flag sets. Plain enums with a bit per flag, combined with |, as the call sites are used to.

    template <typename Flags>
    struct EnableFlagOperators : std::false_type {};

    template <typename Flags> requires EnableFlagOperators<Flags>::value
    constexpr Flags operator|(Flags left, Flags right) {
        using Raw = std::underlying_type_t<Flags>;
        return static_cast<Flags>(static_cast<Raw>(left) | static_cast<Raw>(right));
    }
    template <typename Flags> requires EnableFlagOperators<Flags>::value
    constexpr Flags operator&(Flags left, Flags right) {
        using Raw = std::underlying_type_t<Flags>;
        return static_cast<Flags>(static_cast<Raw>(left) & static_cast<Raw>(right));
    }
    template <typename Flags> requires EnableFlagOperators<Flags>::value
    constexpr Flags operator~(Flags value) {
        using Raw = std::underlying_type_t<Flags>;
        return static_cast<Flags>(~static_cast<Raw>(value));
    }
    template <typename Flags> requires EnableFlagOperators<Flags>::value
    constexpr Flags& operator|=(Flags& left, Flags right) { return left = left | right; }
    template <typename Flags> requires EnableFlagOperators<Flags>::value
    constexpr Flags& operator&=(Flags& left, Flags right) { return left = left & right; }
    template <typename Flags> requires EnableFlagOperators<Flags>::value
    constexpr bool hasFlag(Flags value, Flags flag) {
        using Raw = std::underlying_type_t<Flags>;
        return (static_cast<Raw>(value) & static_cast<Raw>(flag)) != 0;
    }

#define QAPLA_UI_FLAGS(Flags) \
    template <> struct EnableFlagOperators<Flags> : std::true_type {}

    enum class WindowFlags : std::uint32_t {
        None = 0,
        NoTitleBar = 1U << 0U,
        NoResize = 1U << 1U,
        NoMove = 1U << 2U,
        NoScrollbar = 1U << 3U,
        NoScrollWithMouse = 1U << 4U,
        NoCollapse = 1U << 5U,
        AlwaysVerticalScrollbar = 1U << 6U,
        AlwaysHorizontalScrollbar = 1U << 7U,
        HorizontalScrollbar = 1U << 8U,
        NoBringToFrontOnFocus = 1U << 9U,
        NoNavFocus = 1U << 10U,
        NoDecoration = NoTitleBar | NoResize | NoScrollbar | NoCollapse
    };
    QAPLA_UI_FLAGS(WindowFlags);

    enum class ChildFlags : std::uint32_t {
        None = 0,
        Border = 1U << 0U,
        AutoResizeX = 1U << 1U,
        AutoResizeY = 1U << 2U
    };
    QAPLA_UI_FLAGS(ChildFlags);

    enum class TreeNodeFlags : std::uint32_t {
        None = 0,
        DefaultOpen = 1U << 0U,
        Leaf = 1U << 1U,
        Selected = 1U << 2U
    };
    QAPLA_UI_FLAGS(TreeNodeFlags);

    enum class TabBarFlags : std::uint32_t {
        None = 0,
        Reorderable = 1U << 0U
    };
    QAPLA_UI_FLAGS(TabBarFlags);

    enum class TabItemFlags : std::uint32_t {
        None = 0,
        SetSelected = 1U << 0U,   ///< Select this tab in this frame
        Closable = 1U << 1U,      ///< Show a close button; the tab bar's owner removes the tab
        NoTooltip = 1U << 2U,
        Trailing = 1U << 3U
    };
    QAPLA_UI_FLAGS(TabItemFlags);

    enum class InputTextFlags : std::uint32_t {
        None = 0,
        EnterReturnsTrue = 1U << 0U,      ///< Report a change only when Enter is pressed
        CtrlEnterForNewLine = 1U << 1U,   ///< Multiline: Enter submits, Ctrl+Enter breaks the line
        ReadOnly = 1U << 2U
    };
    QAPLA_UI_FLAGS(InputTextFlags);

    enum class SelectableFlags : std::uint32_t {
        None = 0,
        DontClosePopups = 1U << 0U,
        SpanAllColumns = 1U << 1U,
        AllowItemOverlap = 1U << 2U
    };
    QAPLA_UI_FLAGS(SelectableFlags);

    enum class ComboFlags : std::uint32_t {
        None = 0,
        NoPreview = 1U << 0U   ///< Only the arrow, no field showing the current choice
    };
    QAPLA_UI_FLAGS(ComboFlags);

    enum class TableFlags : std::uint32_t {
        None = 0,
        RowBg = 1U << 0U,
        Borders = 1U << 1U,
        SizingFixedFit = 1U << 2U,
        ScrollX = 1U << 3U,
        ScrollY = 1U << 4U,
        Sortable = 1U << 5U
    };
    QAPLA_UI_FLAGS(TableFlags);

    enum class TableColumnFlags : std::uint32_t {
        None = 0,
        WidthFixed = 1U << 0U,
        WidthStretch = 1U << 1U
    };
    QAPLA_UI_FLAGS(TableColumnFlags);

#undef QAPLA_UI_FLAGS

    /** @brief Accepts or rejects one typed character; installed on a text field. */
    using CharFilter = std::function<bool(unsigned int character)>;

    // ------------------------------------------------------------------------------------------
    // Custom drawing
    // ------------------------------------------------------------------------------------------

    /**
     * @brief Draws shapes and text; what the chess board, the clocks, the charts and the toolbar
     * icons are made of.
     *
     * windowDrawList() hands out the one of the container being described, drawn behind its
     * controls. A frontend may hand painters a list of its own, too -- to make an icon image of a
     * toolbar button, say.
     */
    class DrawList {
    public:
        virtual ~DrawList() = default;

        virtual void addLine(Vec2 from, Vec2 to, Color32 color, float thickness = 1.0F) = 0;
        virtual void addRect(Vec2 min, Vec2 max, Color32 color, float rounding = 0.0F,
            float thickness = 1.0F) = 0;
        virtual void addRectFilled(Vec2 min, Vec2 max, Color32 color, float rounding = 0.0F) = 0;
        virtual void addCircle(Vec2 center, float radius, Color32 color, int segments = 0,
            float thickness = 1.0F) = 0;
        virtual void addCircleFilled(Vec2 center, float radius, Color32 color, int segments = 0) = 0;
        virtual void addTriangleFilled(Vec2 first, Vec2 second, Vec2 third, Color32 color) = 0;
        /** @brief Text in the current font at its current size. */
        virtual void addText(Vec2 position, Color32 color, std::string_view text) = 0;
        virtual void addText(Font font, float size, Vec2 position, Color32 color,
            std::string_view text) = 0;
        /** @brief Adds an arc to the current path; angles in radians, 0 pointing right. */
        virtual void pathArcTo(Vec2 center, float radius, float fromAngle, float toAngle,
            int segments = 0) = 0;
        /** @brief Strokes and clears the current path. */
        virtual void pathStroke(Color32 color, bool closed, float thickness = 1.0F) = 0;
        virtual void pushClipRect(Vec2 min, Vec2 max, bool intersectWithCurrent = true) = 0;
        virtual void popClipRect() = 0;
    };

    /** @brief The draw list of the container being described. */
    DrawList& windowDrawList();

    /** @brief Paints an icon into the rectangle at topLeft of the given size. */
    using IconPainter = std::function<void(DrawList& drawList, Vec2 topLeft, Vec2 size)>;

    // ------------------------------------------------------------------------------------------
    // Layout
    // ------------------------------------------------------------------------------------------

    /** @brief Places the next item on the line of the last one, rather than below it. */
    void sameLine(float offsetFromStartX = 0.0F, float spacing = -1.0F);
    void newLine();
    void spacing();
    void separator();
    void indent(float width = 0.0F);
    void unindent(float width = 0.0F);
    /** @brief Takes up space without showing anything. */
    void dummy(Vec2 size);
    void beginGroup();
    void endGroup();

    /** @brief Width of the next control; negative means "all but this much of the free width". */
    void setNextItemWidth(float width);
    void pushItemWidth(float width);
    void popItemWidth();
    /** @brief The width the next control would get. */
    float calcItemWidth();

    /** @brief The free space from the cursor to the far edges of the current container. */
    Vec2 getContentRegionAvail();
    Vec2 getCursorPos();             ///< Relative to the current container
    float getCursorPosX();
    float getCursorPosY();
    void setCursorPos(Vec2 position);
    void setCursorPosX(float x);
    void setCursorPosY(float y);
    Vec2 getCursorScreenPos();       ///< In window coordinates, as drawing uses them
    void setCursorScreenPos(Vec2 position);

    float getFrameHeight();               ///< Height of a framed control: a button, a field
    float getTextLineHeight();
    float getTextLineHeightWithSpacing();
    float getFontSize();
    /** @brief Size of a text in the current font; wrapped at wrapWidth if that is positive. */
    Vec2 calcTextSize(std::string_view text, float wrapWidth = -1.0F);
    Vec2 calcTextSize(Font font, float size, std::string_view text, float wrapWidth = -1.0F);

    const Style& getStyle();

    /** @brief Size of the application's window. */
    Vec2 getViewportSize();
    /** @brief Top left corner of the area the application may draw in, below any title or menu. */
    Vec2 getViewportPos();

    // ------------------------------------------------------------------------------------------
    // Identifiers, style and state stacks
    // ------------------------------------------------------------------------------------------

    void pushId(std::string_view id);
    void pushId(int id);
    void pushId(const void* id);
    void popId();

    void pushStyleColor(StyleColor which, Color32 color);
    void pushStyleColor(StyleColor which, const Vec4& color);
    void popStyleColor(int count = 1);
    void pushStyleVar(StyleVar which, float value);
    void pushStyleVar(StyleVar which, Vec2 value);
    void popStyleVar(int count = 1);
    Color32 getColorU32(StyleColor which, float alphaMultiplier = 1.0F);
    Color32 getColorU32(const Vec4& color);
    Vec4 getStyleColorVec4(StyleColor which);

    void pushFont(Font font);
    void popFont();
    /** @brief The font in use; Font::Inter unless something was pushed. */
    Font getFont();
    /** @brief Scales all text of the current container. */
    void setWindowFontScale(float scale);

    void beginDisabled(bool disabled = true);
    void endDisabled();

    /** @brief Wraps text at this x position of the current container; 0 wraps at its edge. */
    void pushTextWrapPos(float wrapPosX = 0.0F);
    void popTextWrapPos();

    // ------------------------------------------------------------------------------------------
    // Text
    // ------------------------------------------------------------------------------------------

    void text(std::string_view text);
    void textColored(const Vec4& color, std::string_view text);
    void textDisabled(std::string_view text);
    void textWrapped(std::string_view text);
    /** @brief A bullet point; the next item follows on the same line. */
    void bullet();

    // ------------------------------------------------------------------------------------------
    // Controls. Each returns true when the user changed or clicked it since the last frame.
    // ------------------------------------------------------------------------------------------

    bool button(std::string_view label, Vec2 size = Vec2());
    bool smallButton(std::string_view label);
    /** @brief An area that reacts to the mouse and shows nothing; drawing it is up to the caller. */
    bool invisibleButton(std::string_view id, Vec2 size);
    bool checkbox(std::string_view label, bool& value);
    bool radioButton(std::string_view label, bool active);

    bool inputText(std::string_view label, std::string& value,
        InputTextFlags flags = InputTextFlags::None, const CharFilter& filter = nullptr);
    bool inputTextMultiline(std::string_view label, std::string& value, Vec2 size = Vec2(),
        InputTextFlags flags = InputTextFlags::None);
    bool inputInt(std::string_view label, int& value, int step = 1, int stepFast = 100);
    bool inputFloat(std::string_view label, float& value, float step = 0.0F,
        float stepFast = 0.0F, const char* format = "%.3f");
    bool inputU32(std::string_view label, std::uint32_t& value);
    bool sliderInt(std::string_view label, int& value, int min, int max,
        const char* format = "%d");
    /** @brief A progress bar, fraction from 0 to 1. A width of 0 or less fills the line. */
    void progressBar(float fraction, Vec2 size = Vec2(-1.0F, 0.0F),
        std::string_view overlay = {});

    /**
     * @brief A drop-down list. The items follow as selectable() calls, closed by endCombo().
     *
     * Only when this returns true. A toolkit that needs the items while the list is closed --
     * to fill a native pop-up button -- returns true every frame and gathers them.
     */
    bool beginCombo(std::string_view label, std::string_view preview,
        ComboFlags flags = ComboFlags::None);
    void endCombo();
    /** @brief An entry of a combo, a popup or a list; true when it was chosen. */
    bool selectable(std::string_view label, bool selected = false,
        SelectableFlags flags = SelectableFlags::None, Vec2 size = Vec2());
    /** @brief Within a combo: open the list scrolled to the last item. */
    void setItemDefaultFocus();

    /** @brief A section header; true while it is open and its content should be described. */
    bool collapsingHeader(std::string_view label, TreeNodeFlags flags = TreeNodeFlags::None);

    /**
     * @brief A toolbar button: an icon with a label underneath.
     *
     * @param state Highlighting of QaplaButton::ButtonState, passed as its number:
     *              0 normal, 1 active, 2 highlighted, 3 disabled, 4 animated.
     */
    bool iconButton(std::string_view id, std::string_view label, Vec2 size, int state,
        const IconPainter& painter);

    /**
     * @brief The handle between two panes, dragged to share the space between them.
     *
     * @param dragsVertically True for a handle between a top and a bottom pane.
     * @param interactive False shows the handle without letting it be dragged.
     * @return How far it was dragged since the last frame, in pixels along its axis.
     */
    float splitter(std::string_view id, Vec2 size, bool dragsVertically, bool interactive = true);

    // ------------------------------------------------------------------------------------------
    // About the item just described
    // ------------------------------------------------------------------------------------------

    bool isItemHovered();
    bool isItemActive();
    bool isItemClicked(MouseButton button = MouseButton::Left);
    bool isItemVisible();
    bool isItemDeactivatedAfterEdit();
    Vec2 getItemRectMin();
    Vec2 getItemRectMax();
    /** @brief Shows a tooltip now; call it while the item is hovered. */
    void setTooltip(std::string_view text);
    /** @brief Attaches a tooltip to the item just described. */
    void itemTooltip(std::string_view text);

    // ------------------------------------------------------------------------------------------
    // Containers
    // ------------------------------------------------------------------------------------------

    /**
     * @brief A rectangular region of its own, scrolled independently.
     *
     * A size component of 0 takes all the free space, a negative one all but that much.
     * Call endChild() whatever this returns.
     */
    bool beginChild(std::string_view id, Vec2 size = Vec2(), ChildFlags childFlags = ChildFlags::None,
        WindowFlags windowFlags = WindowFlags::None);
    void endChild();

    /**
     * @brief A container at a fixed place of the window, above everything else: the root that
     * fills the window, a notification.
     */
    void beginOverlay(std::string_view id, Vec2 position, Vec2 size,
        WindowFlags flags = WindowFlags::NoDecoration);
    void endOverlay();

    float getScrollY();
    void setScrollY(float scrollY);
    float getScrollMaxX();
    float getScrollMaxY();
    /** @brief Scrolls the current container so the cursor is at ratio of its height. */
    void setScrollHereY(float centerYRatio = 0.5F);
    float getWindowWidth();
    float getWindowHeight();
    bool isWindowFocused();

    // ------------------------------------------------------------------------------------------
    // Popups
    // ------------------------------------------------------------------------------------------

    void openPopup(std::string_view id);
    /** @brief A popup at the mouse; true while it is open. Close it with endPopup(). */
    bool beginPopup(std::string_view id);
    /**
     * @brief A modal dialog; true while it is open. Close it with endPopup().
     *
     * @param open If given, the dialog has a close button that sets it to false.
     * @param initialSize Size the dialog opens with, if positive.
     */
    bool beginPopupModal(std::string_view title, bool* open = nullptr, Vec2 initialSize = Vec2());
    void endPopup();
    void closeCurrentPopup();

    /** @brief One entry of popupMenu(). */
    struct MenuItem {
        std::string label;   ///< Already translated
        bool highlighted = false;
        bool enabled = true;
    };

    /**
     * @brief A menu of commands, opened with openPopup(id).
     *
     * @return The index of the command chosen in this frame.
     */
    std::optional<std::size_t> popupMenu(std::string_view id, const std::vector<MenuItem>& items);

    // ------------------------------------------------------------------------------------------
    // Tabs
    // ------------------------------------------------------------------------------------------

    bool beginTabBar(std::string_view id, TabBarFlags flags = TabBarFlags::None);
    void endTabBar();
    /**
     * @brief A tab; true while it is the selected one, then its content follows and endTabItem().
     *
     * @param open For a closable tab, set to false when the user closed it.
     * @param highlighted Marks the tab, a dot on it: something there asks for attention.
     * @param tooltip Shown while the mouse rests on the tab.
     */
    bool beginTabItem(std::string_view label, bool* open = nullptr,
        TabItemFlags flags = TabItemFlags::None, bool highlighted = false,
        std::string_view tooltip = {});
    void endTabItem();
    /** @brief A button among the tabs, the "+" that adds one. */
    bool tabItemButton(std::string_view label, TabItemFlags flags = TabItemFlags::None);

    // ------------------------------------------------------------------------------------------
    // Tables
    // ------------------------------------------------------------------------------------------

    /**
     * @brief Shows a table's rows; QaplaWindows::ImGuiTable::draw() calls it.
     *
     * The table holds rows, sorting, filtering and the current row; this function lays it out
     * and reports a clicked or keyboard-selected row, by its index in the table's rows.
     */
    std::optional<std::size_t> table(QaplaWindows::ImGuiTable& table, Vec2 size);

    // ------------------------------------------------------------------------------------------
    // Input, time, clipboard
    // ------------------------------------------------------------------------------------------

    bool isKeyPressed(Key key, bool repeat = true);
    bool isKeyDown(Key key);
    /** @brief Whether the modifier of copy and paste is held: Command on a Mac, Ctrl elsewhere. */
    bool isShortcutModifierDown();
    bool isMouseClicked(MouseButton button = MouseButton::Left);
    bool isMouseDown(MouseButton button = MouseButton::Left);
    bool isMouseReleased(MouseButton button = MouseButton::Left);
    Vec2 getMousePos();
    Vec2 getMouseDelta();
    void setMouseCursor(MouseCursor cursor);
    /** @brief Whether a text field has the keyboard: shortcuts should leave the keys alone. */
    bool wantsTextInput();

    /** @brief Seconds since the frontend started. */
    double getTime();
    int getFrameCount();
    /** @brief Seconds the last frame took. */
    float getDeltaTime();

    std::string getClipboardText();
    void setClipboardText(std::string_view text);

    /** @brief Asks for another frame soon, for an animation or a change still to be shown. */
    void requestFrame();

    /** @brief The frontend's name, for logs: "imgui", "macos". */
    std::string_view frontendName();

} // namespace QaplaUi
