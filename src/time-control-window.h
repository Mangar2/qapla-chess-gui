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

#include "embedded-window.h"
#include "qapla-tester/time-control.h"

#include <memory>
#include <array>

namespace QaplaWindows {

    /**
     * @brief Displays time control configuration window.
     * Each instance holds its own time control settings (not static).
     */
    class TimeControlWindow : public EmbeddedWindow {
    public:
        
        enum class SelectedTimeControl {
            Blitz = 0,
            Tournament = 1,
            TimePerMove = 2,
            FixedDepth = 3,
            NodesPerMove = 4
        };
        
        struct TimeControlSettings {
            TimeControlSettings() {
                blitzTime.addTimeSegment({0, 1 * 60 * 1000, 0}); // Default Blitz: 1 min + 0 sec increment
                tournamentTime.addTimeSegment({10 * 60 * 1000, 0, 40}); // Default Tournament: 10 min + 0 sec increment + 40 moves
                timePerMove.setMoveTime(10 * 1000); // Default Time per Move: 10 sec per move
                fixedDepth.setDepth(10); // Default Fixed Depth: 10 plies
                nodesPerMove.setNodes(100000); // Default Nodes per Move: 100,000 nodes
            }
            
            QaplaTester::TimeControl blitzTime;       ///< Time control for Blitz games.
            QaplaTester::TimeControl tournamentTime;  ///< Time control for Tournament games.
            QaplaTester::TimeControl timePerMove;     ///< Time control for Time per Move.
            QaplaTester::TimeControl fixedDepth;      ///< Time control for Fixed Depth.
            QaplaTester::TimeControl nodesPerMove;    ///< Time control for Nodes per Move.
            SelectedTimeControl selected = SelectedTimeControl::Blitz;
            
            const QaplaTester::TimeControl& getSelectedTimeControl() const {
                switch (selected) {
                    case SelectedTimeControl::Blitz: return blitzTime;
                    case SelectedTimeControl::Tournament: return tournamentTime;
                    case SelectedTimeControl::TimePerMove: return timePerMove;
                    case SelectedTimeControl::FixedDepth: return fixedDepth;
                    case SelectedTimeControl::NodesPerMove: return nodesPerMove;
                    default:
                        return blitzTime;
                }
            }
            
            static constexpr std::array<const char*, 5> timeControlStrings{
                "Blitz", "Tournament", "TimePerMove", "FixedDepth", "NodesPerMove"
            };
            
            std::string getSelectionString() const {
                return timeControlStrings[static_cast<size_t>(selected)];
            }
            
            void setSelectionFromString(const std::string& selection) {
                for (size_t i = 0; i < timeControlStrings.size(); ++i) {
                    if (selection == timeControlStrings[i]) {
                        selected = static_cast<SelectedTimeControl>(i);
                        return;
                    }
                }
                throw std::invalid_argument("Invalid time control selection: " + selection);
            }
        };
        
        /**
         * @brief Constructs a TimeControlWindow with default settings.
         */
        TimeControlWindow();
        
        /**
         * @brief Virtual destructor (must be implemented in .cpp for forward declaration).
         */
        ~TimeControlWindow() override;

        void draw() override;        
        
        /**
         * @brief Gets the currently selected time control.
         * @return Reference to the selected TimeControl.
         */
        const QaplaTester::TimeControl& getSelectedTimeControl() const;
        
        /**
         * @brief Gets the time control settings for this instance.
         * @return The current TimeControlSettings.
         */
        const TimeControlSettings& getTimeControlSettings() const {
            return timeControlSettings_;
        }
        
        /**
         * @brief Loads time control configuration from the configuration file.
         * @param id Unique identifier for this instance (e.g., "board1").
         */
        void setFromConfiguration(const std::string& id);
        
    /**
     * @brief Updates the configuration file with current settings.
     * @param id Unique identifier for this instance (e.g., "board1").
     */
    void updateConfiguration(const std::string& id) const;
    
private:        std::string computeActiveButtonId() const;

        /**
         * @brief Edits a time segment by providing input fields for its configuration.
         *
         * @param segment The current time segment to be edited.
         * @param blitz If true, limits input to minutes and seconds for the base time and seconds/milliseconds for the increment.
         * @return TimeSegment The updated time segment with the new configuration.
         */
        static QaplaTester::TimeSegment editTimeSegment(const QaplaTester::TimeSegment& segment, bool blitz);



        /**
         * @brief Draws the UI for configuring Blitz time settings.
         *
         * @param currentTimeControl The current time control settings.
         * @return TimeControl The updated time control settings after user interaction.
         */
        static QaplaTester::TimeControl drawBlitzTime(const QaplaTester::TimeControl& currentTimeControl);

        /**
         * @brief Draws the UI for configuring Tournament time settings.
         *
         * @param currentTimeControl The current time control settings.
         * @return TimeControl The updated time control settings after user interaction.
         */
        static QaplaTester::TimeControl drawTournamentTime(const QaplaTester::TimeControl& currentTimeControl);

        /**
         * @brief Draws the UI for configuring Time per Move settings.
         *
         * @param currentTimeControl The current time control settings.
         * @return TimeControl The updated time control settings after user interaction.
         */
        static QaplaTester::TimeControl drawTimePerMove(const QaplaTester::TimeControl& currentTimeControl);

        /**
         * @brief Draws the UI for configuring Fixed Depth settings.
         *
         * @param currentTimeControl The current time control settings.
         * @return TimeControl The updated time control settings after user interaction.
         */
        static QaplaTester::TimeControl drawFixedDepth(const QaplaTester::TimeControl& currentTimeControl);

        /**
         * @brief Draws the UI for configuring Nodes per Move settings.
         *
         * @param currentTimeControl The current time control settings.
         * @return TimeControl The updated time control settings after user interaction.
         */
    static QaplaTester::TimeControl drawNodesPerMove(const QaplaTester::TimeControl& currentTimeControl);

    TimeControlSettings timeControlSettings_;
};} // namespace QaplaWindows
