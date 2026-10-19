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
 * @file imgui-ui-backend.cpp
 * @brief QaplaUi on Dear ImGui.
 *
 * Almost every function is one ImGui call: QaplaUi was cut along ImGui's own lines, so the ImGui
 * build behaves -- and is identified by the ImGui test engine -- exactly as before the
 * application was written against QaplaUi.
 */

#include "imgui-ui-backend.h"

#include "data/chess-font.h"
#include "data/inter-variable.h"
#include "data/ibm-plex-mono-font.h"

#include <imgui_internal.h>

#include <array>
#include <cmath>
#include <string>

using QaplaUi::Vec2;
using QaplaUi::Vec4;
using QaplaUi::Color32;

namespace QaplaImGui {

    namespace {
        std::array<ImFont*, 4> fonts{};

        template <typename Flags>
        bool has(Flags value, Flags flag) {
            return QaplaUi::hasFlag(value, flag);
        }

        /** @brief ImGui wants zero-terminated strings; the views handed in are not. */
        std::string str(std::string_view text) {
            return std::string(text);
        }
    }

    ImGuiCol toIm(QaplaUi::StyleColor color) {
        using enum QaplaUi::StyleColor;
        switch (color) {
        case Text: return ImGuiCol_Text;
        case TextDisabled: return ImGuiCol_TextDisabled;
        case WindowBg: return ImGuiCol_WindowBg;
        case PopupBg: return ImGuiCol_PopupBg;
        case Border: return ImGuiCol_Border;
        case BorderShadow: return ImGuiCol_BorderShadow;
        case FrameBg: return ImGuiCol_FrameBg;
        case Button: return ImGuiCol_Button;
        case ButtonHovered: return ImGuiCol_ButtonHovered;
        case ButtonActive: return ImGuiCol_ButtonActive;
        case Header: return ImGuiCol_Header;
        case HeaderHovered: return ImGuiCol_HeaderHovered;
        case Separator: return ImGuiCol_Separator;
        case TableRowBg: return ImGuiCol_TableRowBg;
        case TabDimmedSelected: return ImGuiCol_TabDimmedSelected;
        }
        return ImGuiCol_Text;
    }

    ImGuiWindowFlags toIm(QaplaUi::WindowFlags flags) {
        using enum QaplaUi::WindowFlags;
        ImGuiWindowFlags result = ImGuiWindowFlags_None;
        if (has(flags, NoTitleBar)) result |= ImGuiWindowFlags_NoTitleBar;
        if (has(flags, NoResize)) result |= ImGuiWindowFlags_NoResize;
        if (has(flags, NoMove)) result |= ImGuiWindowFlags_NoMove;
        if (has(flags, NoScrollbar)) result |= ImGuiWindowFlags_NoScrollbar;
        if (has(flags, NoScrollWithMouse)) result |= ImGuiWindowFlags_NoScrollWithMouse;
        if (has(flags, NoCollapse)) result |= ImGuiWindowFlags_NoCollapse;
        if (has(flags, AlwaysVerticalScrollbar)) result |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
        if (has(flags, AlwaysHorizontalScrollbar)) result |= ImGuiWindowFlags_AlwaysHorizontalScrollbar;
        if (has(flags, HorizontalScrollbar)) result |= ImGuiWindowFlags_HorizontalScrollbar;
        if (has(flags, NoBringToFrontOnFocus)) result |= ImGuiWindowFlags_NoBringToFrontOnFocus;
        if (has(flags, NoNavFocus)) result |= ImGuiWindowFlags_NoNavFocus;
        return result;
    }

    ImGuiChildFlags toIm(QaplaUi::ChildFlags flags) {
        using enum QaplaUi::ChildFlags;
        ImGuiChildFlags result = ImGuiChildFlags_None;
        if (has(flags, Border)) result |= ImGuiChildFlags_Borders;
        if (has(flags, AutoResizeX)) result |= ImGuiChildFlags_AutoResizeX;
        if (has(flags, AutoResizeY)) result |= ImGuiChildFlags_AutoResizeY;
        return result;
    }

    ImGuiTableFlags toIm(QaplaUi::TableFlags flags) {
        using enum QaplaUi::TableFlags;
        ImGuiTableFlags result = ImGuiTableFlags_None;
        if (has(flags, RowBg)) result |= ImGuiTableFlags_RowBg;
        if (has(flags, Borders)) result |= ImGuiTableFlags_Borders;
        if (has(flags, SizingFixedFit)) result |= ImGuiTableFlags_SizingFixedFit;
        if (has(flags, ScrollX)) result |= ImGuiTableFlags_ScrollX;
        if (has(flags, ScrollY)) result |= ImGuiTableFlags_ScrollY;
        if (has(flags, Sortable)) result |= ImGuiTableFlags_Sortable;
        return result;
    }

    ImGuiTableColumnFlags toIm(QaplaUi::TableColumnFlags flags) {
        using enum QaplaUi::TableColumnFlags;
        ImGuiTableColumnFlags result = ImGuiTableColumnFlags_None;
        if (has(flags, WidthFixed)) result |= ImGuiTableColumnFlags_WidthFixed;
        if (has(flags, WidthStretch)) result |= ImGuiTableColumnFlags_WidthStretch;
        return result;
    }

    static ImGuiTreeNodeFlags toIm(QaplaUi::TreeNodeFlags flags) {
        using enum QaplaUi::TreeNodeFlags;
        ImGuiTreeNodeFlags result = ImGuiTreeNodeFlags_None;
        if (has(flags, DefaultOpen)) result |= ImGuiTreeNodeFlags_DefaultOpen;
        if (has(flags, Leaf)) result |= ImGuiTreeNodeFlags_Leaf;
        if (has(flags, Selected)) result |= ImGuiTreeNodeFlags_Selected;
        return result;
    }

    static ImGuiTabItemFlags toIm(QaplaUi::TabItemFlags flags) {
        using enum QaplaUi::TabItemFlags;
        ImGuiTabItemFlags result = ImGuiTabItemFlags_None;
        if (has(flags, SetSelected)) result |= ImGuiTabItemFlags_SetSelected;
        // Closing a tab is its owner's business: it removes the tab from its list, and until then
        // the tab stays where it is.
        if (has(flags, Closable)) result |= ImGuiTabItemFlags_NoAssumedClosure;
        if (has(flags, NoTooltip)) result |= ImGuiTabItemFlags_NoTooltip;
        if (has(flags, Trailing)) result |= ImGuiTabItemFlags_Trailing;
        return result;
    }

    static ImGuiSelectableFlags toIm(QaplaUi::SelectableFlags flags) {
        using enum QaplaUi::SelectableFlags;
        ImGuiSelectableFlags result = ImGuiSelectableFlags_None;
        if (has(flags, DontClosePopups)) result |= ImGuiSelectableFlags_NoAutoClosePopups;
        if (has(flags, SpanAllColumns)) result |= ImGuiSelectableFlags_SpanAllColumns;
        if (has(flags, AllowItemOverlap)) result |= ImGuiSelectableFlags_AllowOverlap;
        return result;
    }

    static ImGuiKey toIm(QaplaUi::Key key) {
        using enum QaplaUi::Key;
        switch (key) {
        case UpArrow: return ImGuiKey_UpArrow;
        case DownArrow: return ImGuiKey_DownArrow;
        case LeftArrow: return ImGuiKey_LeftArrow;
        case RightArrow: return ImGuiKey_RightArrow;
        case PageUp: return ImGuiKey_PageUp;
        case PageDown: return ImGuiKey_PageDown;
        case Home: return ImGuiKey_Home;
        case End: return ImGuiKey_End;
        case Enter: return ImGuiKey_Enter;
        case Escape: return ImGuiKey_Escape;
        case Delete: return ImGuiKey_Delete;
        case Backspace: return ImGuiKey_Backspace;
        case Tab: return ImGuiKey_Tab;
        case A: return ImGuiKey_A;
        case C: return ImGuiKey_C;
        case V: return ImGuiKey_V;
        case X: return ImGuiKey_X;
        case Z: return ImGuiKey_Z;
        case LeftCtrl: return ImGuiKey_LeftCtrl;
        case RightCtrl: return ImGuiKey_RightCtrl;
        case LeftShift: return ImGuiKey_LeftShift;
        case RightShift: return ImGuiKey_RightShift;
        case LeftAlt: return ImGuiKey_LeftAlt;
        case RightAlt: return ImGuiKey_RightAlt;
        case LeftSuper: return ImGuiKey_LeftSuper;
        case RightSuper: return ImGuiKey_RightSuper;
        }
        return ImGuiKey_None;
    }

    static ImGuiStyleVar toIm(QaplaUi::StyleVar var) {
        using enum QaplaUi::StyleVar;
        switch (var) {
        case WindowPadding: return ImGuiStyleVar_WindowPadding;
        case WindowRounding: return ImGuiStyleVar_WindowRounding;
        case WindowBorderSize: return ImGuiStyleVar_WindowBorderSize;
        case FramePadding: return ImGuiStyleVar_FramePadding;
        case ItemSpacing: return ImGuiStyleVar_ItemSpacing;
        }
        return ImGuiStyleVar_WindowPadding;
    }

    void loadFonts() {
        auto& imguiIO = ImGui::GetIO();

        fonts[static_cast<size_t>(QaplaUi::Font::System)] = imguiIO.Fonts->AddFontDefault();
        ImFontConfig fontCfg;
        fontCfg.FontDataOwnedByAtlas = false;

        static constexpr float internalSizeInPixel = 32.0F;
        fonts[static_cast<size_t>(QaplaUi::Font::Chess)] = imguiIO.Fonts->AddFontFromMemoryTTF(
            reinterpret_cast<void*>(const_cast<uint32_t*>(chessFontData)),
            static_cast<int>(chessFontSize),
            internalSizeInPixel,
            &fontCfg
        );

        static constexpr float interVariableSizeInPixel = 16.0F;
        fonts[static_cast<size_t>(QaplaUi::Font::Inter)] = imguiIO.Fonts->AddFontFromMemoryTTF(
            reinterpret_cast<void*>(const_cast<uint32_t*>(interVariableData)),
            static_cast<int>(interVariableSize),
            interVariableSizeInPixel,
            &fontCfg
        );

        static constexpr float ibmPlexMonoSizeInPixel = 17.0F;
        fonts[static_cast<size_t>(QaplaUi::Font::Mono)] = imguiIO.Fonts->AddFontFromMemoryTTF(
            reinterpret_cast<void*>(const_cast<uint32_t*>(ibmPlexMonoFontData)),
            static_cast<int>(ibmPlexMonoFontSize),
            ibmPlexMonoSizeInPixel,
            &fontCfg
        );

        imguiIO.FontDefault = fonts[static_cast<size_t>(QaplaUi::Font::Inter)];
    }

    ImFont* font(QaplaUi::Font which) {
        auto* result = fonts[static_cast<size_t>(which)];
        return result != nullptr ? result : ImGui::GetFont();
    }

    // ------------------------------------------------------------------------------------------
    // DrawListAdapter
    // ------------------------------------------------------------------------------------------

    void DrawListAdapter::addLine(Vec2 from, Vec2 to, Color32 color, float thickness) {
        list_->AddLine(toIm(from), toIm(to), color, thickness);
    }
    void DrawListAdapter::addRect(Vec2 min, Vec2 max, Color32 color, float rounding, float thickness) {
        list_->AddRect(toIm(min), toIm(max), color, rounding, ImDrawFlags_None, thickness);
    }
    void DrawListAdapter::addRectFilled(Vec2 min, Vec2 max, Color32 color, float rounding) {
        list_->AddRectFilled(toIm(min), toIm(max), color, rounding);
    }
    void DrawListAdapter::addCircle(Vec2 center, float radius, Color32 color, int segments,
        float thickness) {
        list_->AddCircle(toIm(center), radius, color, segments, thickness);
    }
    void DrawListAdapter::addCircleFilled(Vec2 center, float radius, Color32 color, int segments) {
        list_->AddCircleFilled(toIm(center), radius, color, segments);
    }
    void DrawListAdapter::addTriangleFilled(Vec2 first, Vec2 second, Vec2 third, Color32 color) {
        list_->AddTriangleFilled(toIm(first), toIm(second), toIm(third), color);
    }
    void DrawListAdapter::addText(Vec2 position, Color32 color, std::string_view text) {
        list_->AddText(toIm(position), color, text.data(), text.data() + text.size());
    }
    void DrawListAdapter::addText(QaplaUi::Font which, float size, Vec2 position, Color32 color,
        std::string_view text) {
        list_->AddText(font(which), size, toIm(position), color, text.data(),
            text.data() + text.size());
    }
    void DrawListAdapter::pathArcTo(Vec2 center, float radius, float fromAngle, float toAngle,
        int segments) {
        list_->PathArcTo(toIm(center), radius, fromAngle, toAngle, segments);
    }
    void DrawListAdapter::pathStroke(Color32 color, bool closed, float thickness) {
        list_->PathStroke(color, closed ? ImDrawFlags_Closed : ImDrawFlags_None, thickness);
    }
    void DrawListAdapter::pushClipRect(Vec2 min, Vec2 max, bool intersectWithCurrent) {
        list_->PushClipRect(toIm(min), toIm(max), intersectWithCurrent);
    }
    void DrawListAdapter::popClipRect() {
        list_->PopClipRect();
    }

} // namespace QaplaImGui

// ----------------------------------------------------------------------------------------------
// QaplaUi
// ----------------------------------------------------------------------------------------------

namespace QaplaUi {

    using QaplaImGui::toIm;
    using QaplaImGui::fromIm;
    using QaplaImGui::str;

    Color32 toColor32(const Vec4& color) {
        return ImGui::ColorConvertFloat4ToU32(toIm(color));
    }

    Vec4 toVec4(Color32 color) {
        return fromIm(ImGui::ColorConvertU32ToFloat4(color));
    }

    DrawList& windowDrawList() {
        static QaplaImGui::DrawListAdapter adapter;
        adapter.setList(ImGui::GetWindowDrawList());
        return adapter;
    }

    // Layout

    void sameLine(float offsetFromStartX, float spacing) { ImGui::SameLine(offsetFromStartX, spacing); }
    void newLine() { ImGui::NewLine(); }
    void spacing() { ImGui::Spacing(); }
    void separator() { ImGui::Separator(); }
    void indent(float width) { ImGui::Indent(width); }
    void unindent(float width) { ImGui::Unindent(width); }
    void dummy(Vec2 size) { ImGui::Dummy(toIm(size)); }
    void beginGroup() { ImGui::BeginGroup(); }
    void endGroup() { ImGui::EndGroup(); }

    void setNextItemWidth(float width) { ImGui::SetNextItemWidth(width); }
    void pushItemWidth(float width) { ImGui::PushItemWidth(width); }
    void popItemWidth() { ImGui::PopItemWidth(); }
    float calcItemWidth() { return ImGui::CalcItemWidth(); }

    Vec2 getContentRegionAvail() { return fromIm(ImGui::GetContentRegionAvail()); }
    Vec2 getCursorPos() { return fromIm(ImGui::GetCursorPos()); }
    float getCursorPosX() { return ImGui::GetCursorPosX(); }
    float getCursorPosY() { return ImGui::GetCursorPosY(); }
    void setCursorPos(Vec2 position) { ImGui::SetCursorPos(toIm(position)); }
    void setCursorPosX(float x) { ImGui::SetCursorPosX(x); }
    void setCursorPosY(float y) { ImGui::SetCursorPosY(y); }
    Vec2 getCursorScreenPos() { return fromIm(ImGui::GetCursorScreenPos()); }
    void setCursorScreenPos(Vec2 position) { ImGui::SetCursorScreenPos(toIm(position)); }

    float getFrameHeight() { return ImGui::GetFrameHeight(); }
    float getTextLineHeight() { return ImGui::GetTextLineHeight(); }
    float getTextLineHeightWithSpacing() { return ImGui::GetTextLineHeightWithSpacing(); }
    float getFontSize() { return ImGui::GetFontSize(); }

    Vec2 calcTextSize(std::string_view text, float wrapWidth) {
        return fromIm(ImGui::CalcTextSize(text.data(), text.data() + text.size(), false, wrapWidth));
    }

    Vec2 calcTextSize(Font which, float size, std::string_view text, float wrapWidth) {
        auto* imFont = QaplaImGui::font(which);
        return fromIm(imFont->CalcTextSizeA(size, FLT_MAX, wrapWidth > 0.0F ? wrapWidth : 0.0F,
            text.data(), text.data() + text.size()));
    }

    const Style& getStyle() {
        static Style style;
        const auto& imStyle = ImGui::GetStyle();
        style.windowPadding = fromIm(imStyle.WindowPadding);
        style.framePadding = fromIm(imStyle.FramePadding);
        style.itemSpacing = fromIm(imStyle.ItemSpacing);
        style.frameRounding = imStyle.FrameRounding;
        style.scrollbarSize = imStyle.ScrollbarSize;
        return style;
    }

    Vec2 getViewportSize() { return fromIm(ImGui::GetMainViewport()->Size); }
    Vec2 getViewportPos() { return fromIm(ImGui::GetMainViewport()->Pos); }

    // Identifiers and stacks

    void pushId(std::string_view id) { ImGui::PushID(id.data(), id.data() + id.size()); }
    void pushId(int id) { ImGui::PushID(id); }
    void pushId(const void* id) { ImGui::PushID(id); }
    void popId() { ImGui::PopID(); }

    void pushStyleColor(StyleColor which, Color32 color) { ImGui::PushStyleColor(toIm(which), color); }
    void pushStyleColor(StyleColor which, const Vec4& color) {
        ImGui::PushStyleColor(toIm(which), toIm(color));
    }
    void popStyleColor(int count) { ImGui::PopStyleColor(count); }
    void pushStyleVar(StyleVar which, float value) { ImGui::PushStyleVar(toIm(which), value); }
    void pushStyleVar(StyleVar which, Vec2 value) { ImGui::PushStyleVar(toIm(which), toIm(value)); }
    void popStyleVar(int count) { ImGui::PopStyleVar(count); }
    Color32 getColorU32(StyleColor which, float alphaMultiplier) {
        return ImGui::GetColorU32(toIm(which), alphaMultiplier);
    }
    Color32 getColorU32(const Vec4& color) { return ImGui::GetColorU32(toIm(color)); }
    Vec4 getStyleColorVec4(StyleColor which) { return fromIm(ImGui::GetStyleColorVec4(toIm(which))); }

    void pushFont(Font which) { ImGui::PushFont(QaplaImGui::font(which)); }
    void popFont() { ImGui::PopFont(); }
    Font getFont() {
        auto* current = ImGui::GetFont();
        for (auto which : { Font::System, Font::Chess, Font::Inter, Font::Mono }) {
            if (QaplaImGui::font(which) == current) {
                return which;
            }
        }
        return Font::Inter;
    }
    void setWindowFontScale(float scale) { ImGui::SetWindowFontScale(scale); }

    void beginDisabled(bool disabled) { ImGui::BeginDisabled(disabled); }
    void endDisabled() { ImGui::EndDisabled(); }

    void pushTextWrapPos(float wrapPosX) { ImGui::PushTextWrapPos(wrapPosX); }
    void popTextWrapPos() { ImGui::PopTextWrapPos(); }

    // Text

    void text(std::string_view text) { ImGui::TextUnformatted(text.data(), text.data() + text.size()); }
    void textColored(const Vec4& color, std::string_view text) {
        ImGui::PushStyleColor(ImGuiCol_Text, toIm(color));
        ImGui::TextUnformatted(text.data(), text.data() + text.size());
        ImGui::PopStyleColor();
    }
    void textDisabled(std::string_view text) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextUnformatted(text.data(), text.data() + text.size());
        ImGui::PopStyleColor();
    }
    void textWrapped(std::string_view text) {
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextUnformatted(text.data(), text.data() + text.size());
        ImGui::PopTextWrapPos();
    }
    void bullet() { ImGui::Bullet(); }

    // Controls

    bool button(std::string_view label, Vec2 size) { return ImGui::Button(str(label).c_str(), toIm(size)); }
    bool smallButton(std::string_view label) { return ImGui::SmallButton(str(label).c_str()); }
    bool invisibleButton(std::string_view id, Vec2 size) {
        return ImGui::InvisibleButton(str(id).c_str(), toIm(size));
    }
    bool checkbox(std::string_view label, bool& value) { return ImGui::Checkbox(str(label).c_str(), &value); }
    bool radioButton(std::string_view label, bool active) {
        return ImGui::RadioButton(str(label).c_str(), active);
    }

    namespace {
        /** @brief What the text callback needs: the string to grow and the filter to ask. */
        struct TextCallbackData {
            std::string* value;
            const CharFilter* filter;
        };

        int textCallback(ImGuiInputTextCallbackData* data) {
            auto* user = static_cast<TextCallbackData*>(data->UserData);
            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                user->value->resize(static_cast<size_t>(data->BufTextLen));
                data->Buf = user->value->data();
                return 0;
            }
            if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
                return (user->filter != nullptr && *user->filter && !(*user->filter)(data->EventChar))
                    ? 1 : 0;
            }
            return 0;
        }

        ImGuiInputTextFlags toIm(InputTextFlags flags) {
            ImGuiInputTextFlags result = ImGuiInputTextFlags_None;
            if (hasFlag(flags, InputTextFlags::EnterReturnsTrue)) result |= ImGuiInputTextFlags_EnterReturnsTrue;
            if (hasFlag(flags, InputTextFlags::CtrlEnterForNewLine)) result |= ImGuiInputTextFlags_CtrlEnterForNewLine;
            if (hasFlag(flags, InputTextFlags::ReadOnly)) result |= ImGuiInputTextFlags_ReadOnly;
            return result;
        }
    }

    bool inputText(std::string_view label, std::string& value, InputTextFlags flags,
        const CharFilter& filter) {
        // The caller's value follows the field as it is typed in, as a buffer handed to ImGui
        // does; with EnterReturnsTrue only the return value waits for Enter.
        std::string buffer = value;
        TextCallbackData data{ &buffer, &filter };
        auto imFlags = toIm(flags) | ImGuiInputTextFlags_CallbackResize;
        if (filter) {
            imFlags |= ImGuiInputTextFlags_CallbackCharFilter;
        }
        bool changed = ImGui::InputText(str(label).c_str(), buffer.data(), buffer.capacity() + 1,
            imFlags, textCallback, &data);
        if (std::string_view(buffer.c_str()) != value) {
            value = buffer.c_str();
        }
        return changed;
    }

    bool inputTextMultiline(std::string_view label, std::string& value, Vec2 size,
        InputTextFlags flags) {
        std::string buffer = value;
        CharFilter none;
        TextCallbackData data{ &buffer, &none };
        bool changed = ImGui::InputTextMultiline(str(label).c_str(), buffer.data(),
            buffer.capacity() + 1, toIm(size), toIm(flags) | ImGuiInputTextFlags_CallbackResize,
            textCallback, &data);
        if (std::string_view(buffer.c_str()) != value) {
            value = buffer.c_str();
        }
        return changed;
    }

    bool inputInt(std::string_view label, int& value, int step, int stepFast) {
        return ImGui::InputInt(str(label).c_str(), &value, step, stepFast);
    }

    bool inputFloat(std::string_view label, float& value, float step, float stepFast,
        const char* format) {
        return ImGui::InputFloat(str(label).c_str(), &value, step, stepFast, format);
    }

    bool inputU32(std::string_view label, std::uint32_t& value) {
        return ImGui::InputScalar(str(label).c_str(), ImGuiDataType_U32, &value, nullptr, nullptr);
    }

    bool sliderInt(std::string_view label, int& value, int min, int max, const char* format) {
        return ImGui::SliderInt(str(label).c_str(), &value, min, max, format);
    }

    void progressBar(float fraction, Vec2 size, std::string_view overlay) {
        auto overlayText = str(overlay);
        ImGui::ProgressBar(fraction, toIm(size), overlay.empty() ? nullptr : overlayText.c_str());
    }

    bool beginCombo(std::string_view label, std::string_view preview, ComboFlags flags) {
        ImGuiComboFlags imFlags = ImGuiComboFlags_None;
        if (hasFlag(flags, ComboFlags::NoPreview)) {
            imFlags |= ImGuiComboFlags_NoPreview;
        }
        return ImGui::BeginCombo(str(label).c_str(), str(preview).c_str(), imFlags);
    }
    void endCombo() { ImGui::EndCombo(); }

    bool selectable(std::string_view label, bool selected, SelectableFlags flags, Vec2 size) {
        return ImGui::Selectable(str(label).c_str(), selected, QaplaImGui::toIm(flags), toIm(size));
    }
    void setItemDefaultFocus() { ImGui::SetItemDefaultFocus(); }

    bool collapsingHeader(std::string_view label, TreeNodeFlags flags) {
        return ImGui::CollapsingHeader(str(label).c_str(), QaplaImGui::toIm(flags));
    }

    namespace {
        // QaplaButton::ButtonState by number; see iconButton().
        constexpr int BUTTON_ACTIVE = 1;
        constexpr int BUTTON_HIGHLIGHTED = 2;
        constexpr int BUTTON_DISABLED = 3;

        ImU32 iconButtonBgColor(int state) {
            if (state == BUTTON_DISABLED) {
                return ImGui::GetColorU32(ImVec4(0.3F, 0.3F, 0.3F, 0.5F));
            }
            if (ImGui::IsItemActive() || state == BUTTON_ACTIVE) {
                return ImGui::GetColorU32(ImGuiCol_ButtonActive);
            }
            if (ImGui::IsItemHovered()) {
                return ImGui::GetColorU32(ImGuiCol_ButtonHovered);
            }
            return ImGui::GetColorU32(ImGuiCol_Button);
        }

        ImU32 iconButtonTextColor(int state) {
            if (state == BUTTON_DISABLED) {
                return ImGui::GetColorU32(ImGuiCol_TextDisabled);
            }
            return ImGui::GetColorU32(ImGuiCol_Text);
        }
    }

    bool iconButton(std::string_view id, std::string_view label, Vec2 size, int state,
        const IconPainter& painter) {
        ImVec2 topLeft = ImGui::GetCursorScreenPos();
        bool clicked = ImGui::InvisibleButton(str(id).c_str(), toIm(size));

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(topLeft, ImVec2(topLeft.x + size.x, topLeft.y + size.y),
            iconButtonBgColor(state), 3.0F);

        if (painter) {
            QaplaImGui::DrawListAdapter adapter(drawList);
            painter(adapter, fromIm(topLeft), size);
        }

        if (state == BUTTON_HIGHLIGHTED) {
            // draw red dot on top right position
            constexpr float dotRadius = 6.0F;
            constexpr float dotLocation = 2.0F;
            auto dotPos = ImVec2(topLeft.x + size.x - dotLocation, topLeft.y + dotLocation);
            drawList->AddCircleFilled(dotPos, dotRadius, IM_COL32(192, 0, 0, 192));
        }

        auto labelText = str(label);
        ImVec2 labelSize = ImGui::CalcTextSize(labelText.c_str());
        auto labelPos = ImVec2(topLeft.x + size.x * 0.5F - labelSize.x * 0.5F, topLeft.y + size.y + 4.0F);
        drawList->AddText(labelPos, iconButtonTextColor(state), labelText.c_str());

        return clicked;
    }

    float splitter(std::string_view id, Vec2 size, bool dragsVertically, bool interactive) {
        const auto label = "###" + str(id);
        if (!interactive) {
            // A non-interactive splitter with different styling
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(80, 80, 80, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(80, 80, 80, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(80, 80, 80, 255));
            ImGui::Button(label.c_str(), toIm(size));
            ImGui::PopStyleColor(3);
            return 0.0F;
        }

        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(100, 100, 100, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(150, 150, 150, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(180, 180, 180, 255));

        ImGui::Button(label.c_str(), toIm(size));
        float delta = 0.0F;
        if (ImGui::IsItemActive()) {
            delta = dragsVertically ? ImGui::GetIO().MouseDelta.y : ImGui::GetIO().MouseDelta.x;
        }
        if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
            ImGui::SetMouseCursor(dragsVertically ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);
        }

        ImGui::PopStyleColor(3);
        return delta;
    }

    // Items

    bool isItemHovered() { return ImGui::IsItemHovered(); }
    bool isItemActive() { return ImGui::IsItemActive(); }
    bool isItemClicked(MouseButton button) {
        return ImGui::IsItemClicked(static_cast<ImGuiMouseButton>(button));
    }
    bool isItemVisible() { return ImGui::IsItemVisible(); }
    bool isItemDeactivatedAfterEdit() { return ImGui::IsItemDeactivatedAfterEdit(); }
    Vec2 getItemRectMin() { return fromIm(ImGui::GetItemRectMin()); }
    Vec2 getItemRectMax() { return fromIm(ImGui::GetItemRectMax()); }
    void setTooltip(std::string_view text) {
        ImGui::SetTooltip("%s", str(text).c_str());
    }
    void itemTooltip(std::string_view text) {
        if (!text.empty() && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", str(text).c_str());
        }
    }

    // Containers

    bool beginChild(std::string_view id, Vec2 size, ChildFlags childFlags, WindowFlags windowFlags) {
        return ImGui::BeginChild(str(id).c_str(), toIm(size), QaplaImGui::toIm(childFlags),
            QaplaImGui::toIm(windowFlags));
    }
    void endChild() { ImGui::EndChild(); }

    void beginOverlay(std::string_view id, Vec2 position, Vec2 size, WindowFlags flags) {
        ImGui::SetNextWindowPos(toIm(position), ImGuiCond_Always);
        ImGui::SetNextWindowSize(toIm(size), ImGuiCond_Always);
        ImGui::Begin(str(id).c_str(), nullptr, QaplaImGui::toIm(flags));
    }
    void endOverlay() { ImGui::End(); }

    float getScrollY() { return ImGui::GetScrollY(); }
    void setScrollY(float scrollY) { ImGui::SetScrollY(scrollY); }
    float getScrollMaxX() { return ImGui::GetScrollMaxX(); }
    float getScrollMaxY() { return ImGui::GetScrollMaxY(); }
    void setScrollHereY(float centerYRatio) { ImGui::SetScrollHereY(centerYRatio); }
    float getWindowWidth() { return ImGui::GetWindowWidth(); }
    float getWindowHeight() { return ImGui::GetWindowHeight(); }
    bool isWindowFocused() { return ImGui::IsWindowFocused(ImGuiFocusedFlags_None); }

    // Popups

    void openPopup(std::string_view id) { ImGui::OpenPopup(str(id).c_str()); }
    bool beginPopup(std::string_view id) { return ImGui::BeginPopup(str(id).c_str()); }
    bool beginPopupModal(std::string_view title, bool* open, Vec2 initialSize) {
        if (initialSize.x > 0.0F && initialSize.y > 0.0F) {
            ImGui::SetNextWindowSize(toIm(initialSize), ImGuiCond_Once);
        }
        return ImGui::BeginPopupModal(str(title).c_str(), open);
    }
    void endPopup() { ImGui::EndPopup(); }
    void closeCurrentPopup() { ImGui::CloseCurrentPopup(); }

    std::optional<std::size_t> popupMenu(std::string_view id, const std::vector<MenuItem>& items) {
        constexpr float POPUP_WINDOW_PADDING_Y = 4.0F;
        constexpr float POPUP_ITEM_SPACING_Y = 1.0F;
        constexpr float POPUP_ITEM_PADDING_X = 16.0F;
        constexpr float POPUP_ITEM_PADDING_Y = 8.0F;
        constexpr float POPUP_MIN_WIDTH = 180.0F;
        constexpr float HIGHLIGHT_DOT_RADIUS = 4.0F;
        constexpr float HIGHLIGHT_DOT_OFFSET = 6.0F;

        std::optional<std::size_t> selected;

        if (ImGui::BeginPopup(str(id).c_str())) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, POPUP_WINDOW_PADDING_Y));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0F, POPUP_ITEM_SPACING_Y));

            for (std::size_t index = 0; index < items.size(); ++index) {
                const auto& item = items[index];
                ImVec2 itemStartPos = ImGui::GetCursorScreenPos();

                ImVec2 textSize = ImGui::CalcTextSize(item.label.c_str());
                float itemWidth = POPUP_MIN_WIDTH;
                float itemHeight = textSize.y + (POPUP_ITEM_PADDING_Y * 2.0F);

                ImGui::PushID(item.label.c_str());
                bool isClicked = ImGui::InvisibleButton("##item", ImVec2(itemWidth, itemHeight));
                bool isHovered = ImGui::IsItemHovered();
                ImGui::PopID();

                ImDrawList* drawList = ImGui::GetWindowDrawList();
                if (isHovered) {
                    ImU32 bgColor = ImGui::GetColorU32(ImGuiCol_HeaderHovered);
                    drawList->AddRectFilled(itemStartPos,
                        ImVec2(itemStartPos.x + itemWidth, itemStartPos.y + itemHeight),
                        bgColor);
                }

                auto textPos = ImVec2(itemStartPos.x + POPUP_ITEM_PADDING_X,
                    itemStartPos.y + POPUP_ITEM_PADDING_Y);
                ImU32 textColor = ImGui::GetColorU32(item.enabled ? ImGuiCol_Text : ImGuiCol_TextDisabled);
                drawList->AddText(textPos, textColor, item.label.c_str());

                if (item.highlighted) {
                    auto dotPos = ImVec2(itemStartPos.x + HIGHLIGHT_DOT_OFFSET,
                        itemStartPos.y + itemHeight * 0.5F);
                    drawList->AddCircleFilled(dotPos, HIGHLIGHT_DOT_RADIUS, IM_COL32(192, 0, 0, 192));
                }

                if (isClicked && item.enabled) {
                    selected = index;
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::PopStyleVar(2);
            ImGui::EndPopup();
        }

        return selected;
    }

    // Tabs

    namespace {
        void drawDotOnItem(float offsetX, float offsetY) {
            if (ImGui::IsItemVisible()) {
                ImVec2 tabMin = ImGui::GetItemRectMin();
                ImVec2 tabMax = ImGui::GetItemRectMax();
                constexpr float dotRadius = 6.0F;
                auto dotPos = ImVec2(tabMax.x - offsetX, tabMin.y + offsetY);
                ImGui::GetWindowDrawList()->AddCircleFilled(dotPos, dotRadius, IM_COL32(192, 0, 0, 192));
            }
        }
    }

    bool beginTabBar(std::string_view id, TabBarFlags flags) {
        ImGuiTabBarFlags imFlags = ImGuiTabBarFlags_None;
        if (hasFlag(flags, TabBarFlags::Reorderable)) {
            imFlags |= ImGuiTabBarFlags_Reorderable;
        }
        return ImGui::BeginTabBar(str(id).c_str(), imFlags);
    }
    void endTabBar() { ImGui::EndTabBar(); }

    bool beginTabItem(std::string_view label, bool* open, TabItemFlags flags, bool highlighted,
        std::string_view tooltip) {
        bool active = ImGui::BeginTabItem(str(label).c_str(), open, QaplaImGui::toIm(flags));
        if (highlighted) {
            constexpr float dotOffsetX = 6.0F;
            constexpr float dotOffsetY = 10.0F;
            drawDotOnItem(dotOffsetX, dotOffsetY);
        }
        if (!tooltip.empty() && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", str(tooltip).c_str());
        }
        return active;
    }
    void endTabItem() { ImGui::EndTabItem(); }
    bool tabItemButton(std::string_view label, TabItemFlags flags) {
        return ImGui::TabItemButton(str(label).c_str(), QaplaImGui::toIm(flags));
    }

    // Input

    bool isKeyPressed(Key key, bool repeat) { return ImGui::IsKeyPressed(QaplaImGui::toIm(key), repeat); }
    bool isKeyDown(Key key) { return ImGui::IsKeyDown(QaplaImGui::toIm(key)); }
    bool isShortcutModifierDown() {
        return ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl) ||
            ImGui::IsKeyDown(ImGuiKey_LeftSuper) || ImGui::IsKeyDown(ImGuiKey_RightSuper);
    }
    bool isMouseClicked(MouseButton button) {
        return ImGui::IsMouseClicked(static_cast<ImGuiMouseButton>(button));
    }
    bool isMouseDown(MouseButton button) { return ImGui::IsMouseDown(static_cast<ImGuiMouseButton>(button)); }
    bool isMouseReleased(MouseButton button) {
        return ImGui::IsMouseReleased(static_cast<ImGuiMouseButton>(button));
    }
    Vec2 getMousePos() { return fromIm(ImGui::GetMousePos()); }
    Vec2 getMouseDelta() { return fromIm(ImGui::GetIO().MouseDelta); }
    void setMouseCursor(MouseCursor cursor) {
        switch (cursor) {
        case MouseCursor::Arrow: ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow); break;
        case MouseCursor::TextInput: ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput); break;
        case MouseCursor::ResizeEW: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW); break;
        case MouseCursor::ResizeNS: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS); break;
        case MouseCursor::Hand: ImGui::SetMouseCursor(ImGuiMouseCursor_Hand); break;
        }
    }
    bool wantsTextInput() { return ImGui::GetIO().WantTextInput; }

    double getTime() { return ImGui::GetTime(); }
    int getFrameCount() { return ImGui::GetFrameCount(); }
    float getDeltaTime() { return ImGui::GetIO().DeltaTime; }

    std::string getClipboardText() {
        const char* text = ImGui::GetClipboardText();
        return text != nullptr ? std::string(text) : std::string();
    }
    void setClipboardText(std::string_view text) { ImGui::SetClipboardText(str(text).c_str()); }

    void requestFrame() {
        // The frame rate limiter goes back to its high rate on any input; an animation that has
        // to keep going asks for frames by being drawn, which it is anyway.
    }

    std::string_view frontendName() { return "imgui"; }

} // namespace QaplaUi
