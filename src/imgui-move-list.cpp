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

#include "imgui-move-list.h"
#include "qapla-tester/move-record.h"
#include "qapla-tester/game-record.h"
#include <imgui.h>

#include <sstream>
#include <format>
#include <string>

using namespace QaplaWindows;

ImGuiMoveList::ImGuiMoveList() 
: table_("MoveListTable",
    ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY,
    std::vector<ImGuiTable::ColumnDef>{
        {"Move", ImGuiTableColumnFlags_WidthFixed, 90.0f, false, [](std::string& content, bool& alignRight) {
            if (!content.empty() && content[0] == '.') {
                alignRight = true;
            } else {
                alignRight = false;
            }
        }},
        {"Depth", ImGuiTableColumnFlags_WidthFixed, 50.0f, true},
        { "Time", ImGuiTableColumnFlags_WidthFixed, 80.0f, true },
        { "Eval", ImGuiTableColumnFlags_WidthFixed, 50.0f, true },
        { "PV", ImGuiTableColumnFlags_WidthStretch, 0.0f, false }
})
{
    table_.setAutoScroll(true);  
}

static std::string causeToString(GameEndCause cause) {
    switch (cause) {
    case GameEndCause::Checkmate: return "checkmate";
    case GameEndCause::Stalemate: return "stalemate";
    case GameEndCause::DrawByRepetition: return "3-fold repeat";
    case GameEndCause::DrawByFiftyMoveRule: return "50-move rule";
    case GameEndCause::DrawByInsufficientMaterial: return "no material";
    case GameEndCause::DrawByAgreement: return "draw agreement";
    case GameEndCause::Resignation: return "resignation";
    case GameEndCause::Timeout: return "time forfeit";
    case GameEndCause::IllegalMove: return "illegal move";
    case GameEndCause::Adjudication: return "adjudication";
    case GameEndCause::Forfeit: return "forfeit";
    case GameEndCause::TerminatedByTester: return "terminated";
    case GameEndCause::Disconnected: return "disconnected";
    default: return "unknown";
    }
}

void ImGuiMoveList::setFromGameRecord(const GameRecord& gameRecord) {
    
    auto [changed, updated] = gameRecord.getChangeTracker().checkModification(referenceTracker_);
    referenceTracker_.updateFrom(gameRecord.getChangeTracker());
    if (!updated) return;

    if (changed) {
        table_.clear();
    }

    if (table_.size() == 0) {
        if (gameRecord.getStartPos()) {
            table_.push(std::vector<std::string>{ "Start", "", "", "", "" });
        } else if (!gameRecord.getStartFen().empty()) {
            table_.push(std::vector<std::string>{ "Start (Setup)", "", "", "", "" });
        }   
    }
    const auto& moves = gameRecord.history();
    bool wtm = gameRecord.wtmAtPly(table_.size());
    uint32_t moveNumber = 1 + (gameRecord.halfmoveNoAtPly(table_.size()) / 2);
    // table_.size() is safe because we just added the start position if it was empty
    for (size_t i = table_.size() - 1; i < moves.size(); ++i) {
        if (wtm) {
            table_.push(mkRow(" " + std::to_string(moveNumber) + ". ", moves[i], i));
        } 
        else {
            table_.push(mkRow("...", moves[i], i));
            moveNumber++;
        }
        wtm = !wtm;
    }
    table_.setCurrentRow(gameRecord.nextMoveIndex());
    
    // Synchronize currentPly_ with the game state
    currentPly_ = gameRecord.nextMoveIndex();

    const auto [cause, result] = gameRecord.getGameResult();
    if (result != GameResult::Unterminated) {
        table_.push(std::vector<std::string>{causeToString(cause)});
    }

}

std::optional<size_t> ImGuiMoveList::draw() {
    auto size = ImGui::GetContentRegionAvail();
    auto result = table_.draw(size);
    return result;
}


std::vector<std::string> ImGuiMoveList::mkRow(const std::string& label, const MoveRecord& move, size_t index) {
    std::vector<std::string> row;
    row.push_back(label + move.san);
    if (!label.empty() && label[0] == '.') {
    }

    // Depth
    row.push_back(move.depth == 0 ? "-" : std::to_string(move.depth));

    // Time
    row.push_back(QaplaHelpers::formatMs(move.timeMs, move.timeMs < 60000 ? 1 : 0));

    // Eval
    ImGui::TableSetColumnIndex(3);
    if (move.scoreMate) {
        row.push_back(std::format("{}M{}", *move.scoreMate < 0 ? "-" : "", std::abs(*move.scoreMate)));
    }
    else if (move.scoreCp) {
        row.push_back(std::format("{:.2f}", *move.scoreCp / 100.0f));
    } else {
        row.push_back("-");
	}

    // PV
    row.push_back(move.pv);
    return row;
}
