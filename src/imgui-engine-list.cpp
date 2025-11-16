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

#include "imgui-engine-list.h"
#include "font.h"
#include "imgui-table.h"
#include "imgui-button.h"
#include "imgui-separator.h"
#include "engine-record.h"
#include "move-record.h"
#include "game-record.h"
#include "string-helper.h"
#include "engine-event.h"
#include "qapla-engine/types.h"

#include <imgui.h>
#include <sstream>
#include <string>
#include <format>
#include <memory>
#include <algorithm>

using QaplaTester::GameRecord;
using QaplaTester::MoveRecord;
using QaplaTester::SearchInfo;
using QaplaTester::EngineRecord;

using namespace QaplaWindows;

/**
 * Renders a vertical list of readonly text display fields styled like input boxes,
 * but without using actual ImGui::InputText widgets.
 *
 * @param lines Vector of text lines to display, each as its own styled box.
 */
static void renderReadonlyTextBoxes(const std::vector<std::string>& lines, size_t index) {
    constexpr float boxPaddingX = 4.0F;
    constexpr float boxPaddingY = 2.0F;
    constexpr float boxRounding = 2.0F;
    constexpr ImU32 boxBgColor = IM_COL32(50, 52, 60, 255);
    constexpr ImU32 boxBorderColor = IM_COL32(90, 90, 100, 255);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float width = ImGui::CalcItemWidth();
    float boxHeight = ImGui::GetFrameHeight();
    float spacingY = ImGui::GetStyle().ItemSpacing.y;

    for (size_t i = 0; i < lines.size(); ++i) {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size = ImVec2(width, boxHeight);

        drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), boxBgColor, boxRounding);
        drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), boxBorderColor, boxRounding);
		drawList->PushClipRect(pos, ImVec2(pos.x + size.x - 4.0F, pos.y + size.y), true);
        auto textTopLeft = ImVec2(pos.x + boxPaddingX, pos.y + boxPaddingY);
        ImGui::SetCursorScreenPos(textTopLeft);
        if (i == 0 && index <= 1) {
            FontManager::drawPiece(drawList, index == 0 ? QaplaBasics::WHITE_KING : QaplaBasics::BLACK_KING);
            ImGui::SetCursorScreenPos(ImVec2(textTopLeft.x + 20, textTopLeft.y));
        }
        ImGui::TextUnformatted(lines[i].c_str());
		drawList->PopClipRect();
        ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + size.y + spacingY));
    }
}

static void drawEngineInfo(const EngineRecord& record, size_t index) {
    ImGui::Indent(5.0F);
    ImGui::PushID("EngineInfo");
    std::string name = record.config.getName();
	std::string status = EngineRecord::to_string(record.status);
	std::string memory = record.memoryUsageB ? 
        ", " + std::to_string(*record.memoryUsageB / (1024 * 1024)) + " MB" : "";
	renderReadonlyTextBoxes({ name, status + memory }, index);
    ImGui::PopID();
    ImGui::Unindent(5.0F);
}


ImGuiEngineList::ImGuiEngineList() = default;

ImGuiEngineList::ImGuiEngineList(ImGuiEngineList&&) noexcept = default;
ImGuiEngineList& ImGuiEngineList::operator=(ImGuiEngineList&&) noexcept = default;

ImGuiEngineList::~ImGuiEngineList() = default;

void ImGuiEngineList::setFromGameRecord(const GameRecord& gameRecord) {
    auto [modification, update] = gameRecordTracker_.checkModification(gameRecord.getChangeTracker());
    if (!update) { 
        return; 
    }
    gameRecordTracker_.updateFrom(gameRecord.getChangeTracker());
    auto nextMoveIndex = gameRecord.nextMoveIndex();
    nextHalfmoveNo_ = gameRecord.halfmoveNoAtPly(nextMoveIndex) + 1;
    const auto& history = gameRecord.history();
    // Set the first two engines (white/black) based on the game record
    // Set table checks, if the table index exists
    for (int i = 0; i < 2; i++) {
        int moveIndex = static_cast<int>(nextMoveIndex) - i - 1;
        auto tableIndex = static_cast<size_t>(i);
        // If isWhiteToMove, last move is a black move (tableIndex == 1), so swap table index
        if (engineRecords_.size() >= 2) {
            if (gameRecord.isWhiteToMove()) { tableIndex = 1 - tableIndex; }
        }
        if (tableIndex >= engineRecords_.size()) {
            break; 
        }
        if (moveIndex < 0 || static_cast<size_t>(moveIndex) >= history.size()) {
			if (infoTables_.size() > tableIndex) {
                infoTables_[tableIndex].infoTable_->clear();
            }
            if (moveIndex == -1) {
                displayedMoveNo_[tableIndex] = gameRecord.halfmoveNoAtPly(0);
            }
            continue;
        }
        const auto& moveRecord = history[static_cast<size_t>(moveIndex)];
        setInfoTable(tableIndex, moveRecord);
        displayedMoveNo_[tableIndex] = moveRecord.halfmoveNo_;
    }
}

void ImGuiEngineList::setFromMoveRecord(const MoveRecord& moveRecord, uint32_t playerIndex,
    const std::string& gameStatus) 
{
    addTables(playerIndex + 1);
    if (moveRecord.depth == 0 && moveRecord.nodes == 0) {
        // No search info yet, nothing to display
        return;
    }

    bool analyzeMode = (gameStatus == "Analyze");
    if (!analyzeMode && !shouldDisplayMoveRecord(moveRecord, playerIndex)) {
        return;
    }

    infoCnt_[playerIndex] = moveRecord.infoUpdateCount;
    // Update the displayed move number to the move that is being shown
    // setFromGameRecord will set the halfmoveNumber of already played moves while
    // Here we also show moves currently being calculated
    displayedMoveNo_[playerIndex] = moveRecord.halfmoveNo_;

    setInfoTable(playerIndex, moveRecord);
}

void ImGuiEngineList::setFromLogBuffer(const QaplaTester::RingBuffer& logBuffer, uint32_t playerIndex) {
    addTables(playerIndex + 1);
    auto& infoTable = infoTables_[playerIndex];
    auto [modification, update] = infoTable.logTracker_.checkModification(logBuffer.getChangeTracker());
    if (!update) { 
        return; 
    }
    auto& logTable = infoTable.logTable_;

    if (modification) {
        logTable->clear();
        infoTable.lastInputCount_ = 0;
    }
    
    if (logBuffer.size() == 0) {
        return;
    }
    
    size_t smallestInputCount = logBuffer[0].inputCount;
    size_t largestInputCount = logBuffer[logBuffer.size() - 1].inputCount;
    
    // If the smallest number in the buffer is greater than our last processed number,
    // the buffer has wrapped around and we've lost continuity - clear the table
    if (infoTable.lastInputCount_ + 1 < smallestInputCount) {
        logTable->clear();
        infoTable.lastInputCount_ = 0;
    }
    
    size_t index = infoTable.lastInputCount_ + 1 - smallestInputCount;
    
    // Add only new entries starting from calculated index
    for (; index < logBuffer.size(); ++index) {
        const auto& entry = logBuffer[index];
        auto countStr = std::format("{:05}", entry.inputCount);
        logTable->push({ countStr, entry.data });
    }
    
    // Update lastInputCount to the largest inputCount in buffer
    infoTable.lastInputCount_ = largestInputCount;
}

void ImGuiEngineList::pollLogBuffers() {
    for (size_t i = 0; i < infoTables_.size() && i < engineRecords_.size(); ++i) {
        auto& engineRecord = engineRecords_[i];
        auto& engineId = engineRecord.identifier;
        QaplaTester::EngineLogger::withEngineLogBuffer(engineId, [&](const QaplaTester::RingBuffer &logBuffer) {
            setFromLogBuffer(logBuffer, static_cast<uint32_t>(i));
        });
    }
}

bool QaplaWindows::ImGuiEngineList::shouldDisplayMoveRecord(
    const QaplaTester::MoveRecord &moveRecord, uint32_t playerIndex) 
{
    size_t displayedMoveNo = 0;
    auto moveNo = moveRecord.halfmoveNo_;

    // Only update if this is the current move showed in the table or the next move currently calculated
    if (moveRecord.ponderMove.empty())
    {
        auto opponent = playerIndex == 0 && displayedMoveNo_.size() > 1 ? 1 : 0;
        // The last move came from the opponent thus this is the currently displayed move number on the board
        displayedMoveNo = displayedMoveNo_[opponent];
    } else {
        // When pondering, the last move came from the pondering player thus we use playerIndex
        displayedMoveNo = displayedMoveNo_[playerIndex];
    }

    if (moveNo != displayedMoveNo && moveNo != displayedMoveNo + 1)
    {
        return false;
    }

    // Only update if there are new info records
    if (moveRecord.infoUpdateCount == infoCnt_[playerIndex])
    {
        return false;
    }
    return true;
}

void ImGuiEngineList::addTables(size_t size) {
    for (size_t i = infoTables_.size(); i < size; ++i) {
		displayedMoveNo_.push_back(0);
        gameRecordTracker_.clear();
		infoCnt_.push_back(0);
        infoTables_.emplace_back(
            std::make_unique<ImGuiTable>(std::format("EngineTable{}", i),
                ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit,
                std::vector<ImGuiTable::ColumnDef>{
                    { .name = "Depth", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F, .alignRight = true },
                    { .name = "Time", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F, .alignRight = true },
                    { .name = "Nodes", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 80.0F, .alignRight = true },
                    { .name = "NPS", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 60.0F, .alignRight = true },
                    { .name = "Tb hits", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F, .alignRight = true },
                    { .name = "Value", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 50.0F, .alignRight = true },
                    { .name = "Primary variant", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 1660.0F }
            }),
            std::make_unique<ImGuiTable>(std::format("EngineLogTable{}", i),
                ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit,
                std::vector<ImGuiTable::ColumnDef>{
                    { .name = "Count", .flags = ImGuiTableColumnFlags_WidthFixed, .width = 60.0F, .alignRight = true },
                    { .name = "Log Entry", .flags = ImGuiTableColumnFlags_WidthStretch, .width = 1800.0F }
            })
        );
        infoTables_[i].infoTable_->setClickable(true);
        infoTables_[i].infoTable_->setFont(FontManager::ibmPlexMonoIndex);
        infoTables_[i].logTable_->setSortable(true);
        infoTables_[i].logTable_->setFont(FontManager::ibmPlexMonoIndex);
    }
}

static std::vector<std::string> mkTableLine(ImGuiTable* table, const SearchInfo& info, const std::string& ponderMove) {
        std::string npsStr = "-";
        std::string score = "-";
        std::string pv;
        if (info.nps) {
            npsStr = std::format("{:L}", *info.nps);
        }
        else if (info.timeMs && info.nodes && *info.timeMs != 0) {
            npsStr = std::format("{:L}", static_cast<int64_t>(
                static_cast<double>(*info.nodes) * 1000.0 / static_cast<double>(*info.timeMs)));
        }
        if (info.scoreMate) {
            score = std::format("{}M{}", *info.scoreMate < 0 ? "-" : "", std::abs(*info.scoreMate));
        }
        else if (info.scoreCp) {
            score = std::format("{:.2f}", *info.scoreCp / 100.0F);
        }
        if (score != "-" && table->size() == 1 && table->getField(0, 5)  == "-") {
            table->setField(0, 5, score);
		}
        if (!info.pv.empty()) {
            if (!ponderMove.empty()) { pv = ponderMove + "  "; }
            for (const auto& move : info.pv) {
                if (!pv.empty()) { pv += ' '; }
                pv += move;
            }
        } else if (info.currMove) {
            if (!ponderMove.empty()) { pv = ponderMove + "  "; }
            pv += *info.currMove;
        } 
        std::vector<std::string> row = {
            info.depth ? std::to_string(*info.depth) : "-",
            info.timeMs ? QaplaHelpers::formatMs(*info.timeMs, 0) : "-",
            info.nodes ? std::format("{:L}", *info.nodes) : "-",
            npsStr,
            info.tbhits ? std::format("{:L}", *info.tbhits) : "-",
            score,
            pv
        };
        return row;
}

void ImGuiEngineList::setInfoTable(size_t index, const MoveRecord& moveRecord) {
    
    if (index >= infoTables_.size()) {
        return; 
    }
    
    auto& table = infoTables_[index].infoTable_;
	const auto& searchInfos = moveRecord.info;

    table->clear();

    bool last = true;
    for (size_t i = searchInfos.size(); i > 0; --i) {
        if (table->size() >= searchInfos.size()) { break; }
        const auto& info = searchInfos[i - 1];
        if (info.pv.empty() && !last) { continue; }
        last = false;
        auto row = mkTableLine(table.get(), info, moveRecord.ponderMove);
        table->push(row);
    }
}

static std::string drawButtons(bool showLog) {
    constexpr float space = 3.0F;
    constexpr float topOffset = 5.0F;
    constexpr float bottomOffset = 8.0F;
    constexpr float leftOffset = 20.0F;
    std::vector<std::string> buttons{ "Restart", "Stop", "Log" };
    ImVec2 topLeft = ImGui::GetCursorScreenPos();
	topLeft.x = std::round(topLeft.x);
	topLeft.y = std::round(topLeft.y);
    auto curPos = ImVec2(topLeft.x + leftOffset, topLeft.y + topOffset);

    constexpr ImVec2 buttonSize = { 25.0F, 25.0F };

    auto totalSize = QaplaButton::calcIconButtonsTotalSize(buttonSize, buttons);
    
    std::string command;
    for (const auto& button : buttons) {
        ImGui::SetCursorScreenPos(curPos);
        QaplaButton::ButtonState state = QaplaButton::ButtonState::Normal;
        if (button == "Log" && showLog) {
            state = QaplaButton::ButtonState::Active;
        }
        if (QaplaButton::drawIconButton(button, button, buttonSize, state,
            [&button, &state](ImDrawList* drawList, ImVec2 topLeft, ImVec2 size) {
                if (button == "Restart") {
                    QaplaButton::drawRestart(drawList, topLeft, size, state);
                }
                if (button == "Stop") {
                    QaplaButton::drawStop(drawList, topLeft, size, state);
                }
                if (button == "Log") {
                    QaplaButton::drawLog(drawList, topLeft, size, state);
                }
            }))
        {
            command = button;
        }
        curPos.x += totalSize.x + space;
    }

    ImGui::SetCursorScreenPos(ImVec2(topLeft.x, topLeft.y + totalSize.y + topOffset + bottomOffset));
    return command;
}

std::string ImGuiEngineList::drawEngineSpace(size_t index, ImVec2 size) {

    constexpr float cEngineInfoWidth = 160.0F;
    constexpr float cSectionSpacing = 4.0F;
    
    const bool isSmall = size.y < 100.0F;
    
    const ImU32 bgColor = ImGui::GetColorU32(ImGuiCol_TableRowBg);

    ImVec2 topLeft = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImGui::PushID(std::to_string(index).c_str());
    ImVec2 max = ImVec2(topLeft.x + cEngineInfoWidth + size.x + cSectionSpacing, topLeft.y + size.y);

    drawList->AddRectFilled(topLeft, max, bgColor);
    ImGuiSeparator::Horizontal();

    std::string command = drawEngineArea(topLeft, drawList, max, cEngineInfoWidth, index, isSmall);

    ImGui::SetCursorScreenPos(ImVec2(topLeft.x + cEngineInfoWidth, topLeft.y));
    ImGuiSeparator::Vertical();

    std::string pv;
    if (index < infoTables_.size() && infoTables_[index].showLog_) {
        drawLog(topLeft, cEngineInfoWidth, cSectionSpacing, index, max, size);
    } else {
        // User may select a PV from the engine table
        pv = drawEngineTable(topLeft, cEngineInfoWidth, cSectionSpacing, index, max, size);
    }
    
    ImGui::SetCursorScreenPos(topLeft);
    ImGui::Dummy(ImVec2(size.x, size.y - 3.0F));
    ImGui::PopID();
    return command.empty() ? pv : command;
}

std::string QaplaWindows::ImGuiEngineList::drawEngineArea(const ImVec2 &topLeft, ImDrawList *drawList, 
    const ImVec2 &max, float cEngineInfoWidth, size_t index, bool isSmall)
{
    std::string command;
    ImGui::SetCursorScreenPos(topLeft);
    drawList->PushClipRect(topLeft, max);
    ImGui::SetCursorScreenPos(ImVec2(topLeft.x, topLeft.y + 5.0F));
    ImGui::PushItemWidth(cEngineInfoWidth - 10.0F);
    bool hasEngine = (index < engineRecords_.size());
    auto& infoTable = infoTables_[index];
    if (allowInput_ && (!isSmall || !hasEngine)) { 
        command = drawButtons(infoTable.showLog_); 
        if (command == "Log") {
            infoTable.showLog_ = !infoTable.showLog_;
        }
    }
    if (hasEngine) {
        drawEngineInfo(engineRecords_[index], index);
    }
    ImGui::PopItemWidth();
    drawList->PopClipRect();
    return command;
}

// Encode a PV string in a compact, easy-to-parse format.
// Format: "pv|<halfmoveNo>|<pv>"
static std::string encodePV(uint32_t halfmoveNo, const std::string& pv) {
    return std::string("pv|") + std::to_string(halfmoveNo) + '|' + pv;
}

 std::string QaplaWindows::ImGuiEngineList::drawEngineTable(
    const ImVec2 &topLeft, float cEngineInfoWidth, float cSectionSpacing, 
    size_t index, const ImVec2 &max, const ImVec2 &size)
{
    ImVec2 tableMin = ImVec2(topLeft.x + cEngineInfoWidth + cSectionSpacing, topLeft.y);
    ImGui::SetCursorScreenPos(tableMin);
    std::string pv;
    if (index < infoTables_.size())
    {
        auto tableSize = ImVec2(max.x - tableMin.x, size.y);
        auto& infoTable = infoTables_[index].infoTable_;
        if (ImGui::BeginChild("TableScroll", tableSize, ImGuiChildFlags_AutoResizeX,
            ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar))
        {
            auto clicked = infoTable->draw(ImVec2(2000.0F, tableSize.y));
            if (clicked) {
                pv = encodePV(displayedMoveNo_[index], infoTable->getField(*clicked, 6));
            }
        }
        ImGui::EndChild();
    }
    return pv;
}

 void QaplaWindows::ImGuiEngineList::drawLog(
    const ImVec2 &topLeft, float cEngineInfoWidth, float cSectionSpacing, 
    size_t index, const ImVec2 &max, const ImVec2 &size)
{
    ImVec2 tableMin = ImVec2(topLeft.x + cEngineInfoWidth + cSectionSpacing, topLeft.y);
    ImGui::SetCursorScreenPos(tableMin);
    if (index < infoTables_.size())
    {
        auto tableSize = ImVec2(max.x - tableMin.x, size.y);
        auto& logTable = infoTables_[index].logTable_;
        if (ImGui::BeginChild("TableScroll", tableSize, ImGuiChildFlags_AutoResizeX,
            ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar))
        {
            logTable->draw(ImVec2(2000.0F, tableSize.y));
        }
        ImGui::EndChild();
    }
}

std::pair<std::string, std::string> ImGuiEngineList::draw() {
    const float cMinRowHeight = 50.0F;
    constexpr float cEngineInfoWidth = 160.0F;
    constexpr float cMinTableWidth = 200.0F;
    constexpr float cSectionSpacing = 4.0F;

	const auto records = engineRecords_.size();
    addTables(records);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    const float tableMinWidth = std::max(cMinTableWidth, avail.x - cEngineInfoWidth - cSectionSpacing);
    const uint32_t rowHeight = static_cast<uint32_t>(
        std::max(cMinRowHeight, avail.y / static_cast<float>(records)));
    std::string id;
    std::string command;
    for (size_t i = 0; i < records; ++i) {
		auto c = drawEngineSpace(i, ImVec2(tableMinWidth, static_cast<float>(rowHeight)));
        if (!c.empty()) {
            id = engineRecords_[i].identifier;
            command = std::move(c); 
        }
    }
    return { id, command };
}
