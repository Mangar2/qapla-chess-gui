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

#include <vector>
#include <string>
#include <memory>
#include "qapla-tester/game-record.h"

namespace QaplaWindows {

/**
 * @brief Interface for game filters
 */
class ITableFilter {
public:
    using Row = std::vector<std::string>;
    using Table = std::vector<Row>;
    /**
     * @brief Checks if a game matches the filter criteria.
     * @param game The game record to check.
     * @return True if the game matches the filter, false otherwise.
     */
    virtual bool matches(const Row& row) const = 0;

    /**
     * @brief Renders the filter's configuration UI.
     * @return True if the filter configuration has changed, false otherwise.
     */
    virtual bool draw() = 0;

    /**
     * @brief Updates the filter's options based on the provided game records.
     * @param games The list of game records to analyze.
     */
    virtual void updateOptions(const ITableFilter::Table& table) = 0;

    /**
     * @brief Virtual destructor for the interface.
     */
    virtual ~ITableFilter() = default;
};

/**
 * @brief Full text filter for game records
 */
class FullTextFilter : public ITableFilter {
private:
    std::string searchText_; ///< The current search text.
    bool filterChanged_; ///< Flag indicating if the filter configuration has changed.

public:
    /**
     * @brief Constructs a FullTextFilter with default settings.
     */
    FullTextFilter();

    /**
     * @brief Checks if a game matches the full text search criteria.
     * @param game The game record to check.
     * @return True if the game matches the search text, false otherwise.
     */
    bool matches(const Row& row) const override;

    /**
     * @brief Renders the full text search UI.
     * @return True if the search text has changed, false otherwise.
     */
    bool draw() override;

    /**
     * @brief Updates the filter's options (not used for full text filter).
     * @param games The list of game records (ignored).
     */
    void updateOptions([[maybe_unused]] const ITableFilter::Table& table) override {};

    private:
        /**
         * @brief Sends the current search text to the configuration system.
         */
       void sendOptionsToConfiguration();
};

/**
 * @brief Meta filter that combines all available filters
 */
class MetaFilter : public ITableFilter {
private:
    std::vector<std::unique_ptr<ITableFilter>> filters_; ///< List of all filters managed by the meta filter.

public:
    /**
     * @brief Constructs a MetaFilter and initializes it with available filters.
     */
    MetaFilter();

    /**
     * @brief Checks if a game matches all combined filter criteria.
     * @param game The game record to check.
     * @return True if the game matches all filters, false otherwise.
     */
    bool matches(const Row& row) const override;

    /**
     * @brief Renders the UI for all combined filters.
     * @return True if any filter's configuration has changed, false otherwise.
     */
    bool draw() override;

    /**
     * @brief Updates the options for all combined filters.
     * @param games The list of game records to analyze.
     */
    void updateOptions(const std::vector<Row>& table) override;
};

} // namespace QaplaWindows
