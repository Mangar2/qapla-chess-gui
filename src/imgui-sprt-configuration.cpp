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

#include "imgui-sprt-configuration.h"
#include "imgui-controls.h"
#include "configuration.h"
#include "string-helper.h"
#include "sprt-manager.h"

#include <imgui.h>
#include <string>
#include <algorithm>

using namespace QaplaWindows;

bool ImGuiSprtConfiguration::draw(const DrawOptions& options) {
    if (config_ == nullptr) {
        return false;
    }

    bool changed = false;

    bool shouldDraw = !options.showCollapsingHeader ||
        ImGuiControls::CollapsingHeaderWithDot("SPRT Configuration", ImGuiTreeNodeFlags_Selected);

    if (shouldDraw) {
        ImGui::PushID("sprtConfig");
        ImGui::Indent(options.indent);

        if (options.showEloLower) {
            changed |= drawEloLower(options.inputWidth);
        }
        
        if (options.showEloUpper) {
            changed |= drawEloUpper(options.inputWidth);
        }

        // Validation hint for Elo bounds
        if (config_->eloLower >= config_->eloUpper) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 0.4F, 0.4F, 1.0F));
            ImGuiControls::textWrapped("Elo Lower must be less than Elo Upper.");
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();

        if (options.showAlpha) {
            changed |= drawAlpha(options.inputWidth);
        }
        
        if (options.showBeta) {
            changed |= drawBeta(options.inputWidth);
        }

        ImGui::Spacing();

        if (options.showMaxGames) {
            changed |= drawMaxGames(options.inputWidth);
        }

        ImGui::Unindent(options.indent);
        ImGui::PopID();
    }

    if (changed) {
        updateConfiguration();
    }

    return changed;
}

bool ImGuiSprtConfiguration::drawEloLower(float inputWidth) {
    ImGui::SetNextItemWidth(inputWidth);
    bool changed = ImGuiControls::inputInt<int>("Elo Lower (H0)", config_->eloLower, -1000, 1000);
    ImGuiControls::hooverTooltip(
        "Lower Elo bound (H0): null hypothesis threshold for SPRT test.\n"
        "If true Elo difference is below this, H0 is accepted (no improvement).");
    return changed;
}

bool ImGuiSprtConfiguration::drawEloUpper(float inputWidth) {
    ImGui::SetNextItemWidth(inputWidth);
    bool changed = ImGuiControls::inputInt<int>("Elo Upper (H1)", config_->eloUpper, -1000, 1000);
    ImGuiControls::hooverTooltip(
        "Upper Elo bound (H1): alternative hypothesis threshold for SPRT test.\n"
        "If true Elo difference is above this, H1 is accepted (improvement confirmed).");
    return changed;
}

bool ImGuiSprtConfiguration::drawAlpha(float inputWidth) {
    ImGui::SetNextItemWidth(inputWidth);
    bool changed = ImGuiControls::inputPromille("Alpha (\xe2\x80\xb0)", config_->alpha, 0.001, 0.5, 0.001);
    ImGuiControls::hooverTooltip(
        "Type I error rate (false positive): probability of rejecting H0 when it's true.\n"
        "Lower values mean more confidence but require more games.");
    ImGui::SameLine();
    ImGui::Text("(%.3f)", config_->alpha);
    return changed;
}

bool ImGuiSprtConfiguration::drawBeta(float inputWidth) {
    ImGui::SetNextItemWidth(inputWidth);
    bool changed = ImGuiControls::inputPromille("Beta (\xe2\x80\xb0)", config_->beta, 0.001, 0.5, 0.001);
    ImGuiControls::hooverTooltip(
        "Type II error rate (false negative): probability of accepting H0 when H1 is true.\n"
        "Lower values mean more confidence but require more games.");
    ImGui::SameLine();
    ImGui::Text("(%.3f)", config_->beta);
    return changed;
}

bool ImGuiSprtConfiguration::drawMaxGames(float inputWidth) {
    ImGui::SetNextItemWidth(inputWidth);
    bool changed = ImGuiControls::inputInt<uint32_t>("Max Games", config_->maxGames, 1, 1000000);
    ImGuiControls::hooverTooltip(
        "Maximum number of games before test terminates inconclusively.\n"
        "If neither H0 nor H1 is accepted within this limit, the result is inconclusive.");
    return changed;
}

void ImGuiSprtConfiguration::loadConfiguration() {
    if (config_ == nullptr) {
        return;
    }

    auto& configData = QaplaConfiguration::Configuration::instance().getConfigData();
    auto sections = configData.getSectionList("sprtconfig", id_);
    if (!sections || sections->empty()) {
        return;
    }

    for (const auto& [key, value] : (*sections)[0].entries) {
        if (key == "eloLower") {
            config_->eloLower = QaplaHelpers::to_int(value).value_or(-5);
            config_->eloLower = std::clamp(config_->eloLower, -1000, 1000);
        }
        else if (key == "eloUpper") {
            config_->eloUpper = QaplaHelpers::to_int(value).value_or(5);
            config_->eloUpper = std::clamp(config_->eloUpper, -1000, 1000);
        }
        else if (key == "alpha") {
            try {
                config_->alpha = std::stod(value);
                config_->alpha = std::clamp(config_->alpha, 0.001, 0.5);
            } catch (...) {
                config_->alpha = 0.05;
            }
        }
        else if (key == "beta") {
            try {
                config_->beta = std::stod(value);
                config_->beta = std::clamp(config_->beta, 0.001, 0.5);
            } catch (...) {
                config_->beta = 0.05;
            }
        }
        else if (key == "maxGames") {
            config_->maxGames = QaplaHelpers::to_uint32(value).value_or(100000);
        }
    }
}

std::vector<QaplaHelpers::IniFile::Section> ImGuiSprtConfiguration::getSections() const {
    if (config_ == nullptr) {
        return {};
    }

    QaplaHelpers::IniFile::KeyValueMap entries{
        {"id", id_},
        {"eloLower", std::to_string(config_->eloLower)},
        {"eloUpper", std::to_string(config_->eloUpper)},
        {"alpha", std::to_string(config_->alpha)},
        {"beta", std::to_string(config_->beta)},
        {"maxGames", std::to_string(config_->maxGames)}
    };

    return { QaplaHelpers::IniFile::Section{ .name = "sprtconfig", .entries = entries } };
}

void ImGuiSprtConfiguration::updateConfiguration() const {
    auto sections = getSections();
    QaplaConfiguration::Configuration::instance().getConfigData().setSectionList(
        "sprtconfig", id_, sections);
}

bool ImGuiSprtConfiguration::isValid() const {
    if (config_ == nullptr) {
        return false;
    }

    // Elo Lower must be less than Elo Upper
    if (config_->eloLower >= config_->eloUpper) {
        return false;
    }
    
    // Alpha and Beta must be positive
    if (config_->alpha <= 0.0 || config_->beta <= 0.0) {
        return false;
    }
    
    // Max Games must be at least 1
    if (config_->maxGames < 1) {
        return false;
    }
    
    return true;
}
