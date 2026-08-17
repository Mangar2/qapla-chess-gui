/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Volker Böhm
 * @copyright Copyright (c) 2026 Volker Böhm
 */

#include "gui-tools.h"
#include "gui-tools-shared.h"
#include "../actions/gui-action-clop.h"

#include <charconv>
#include <string>
#include <utility>
#include <vector>

namespace QaplaLlm {

namespace {
    using Actions::ClopParameter;
    using Actions::ClopSettings;
    using Request = ClopSettings;

    /**
     * @brief Reads "0 40000" or "0..40000" into the two numbers of a parameter range.
     * @return False if the text is not two numbers.
     */
    bool parseRange(std::string text, double& minimum, double& maximum) {
        for (auto& character : text) {
            if (character == '.' && text.find("..") != std::string::npos) {
                break;
            }
        }
        const auto separator = text.find("..");
        std::string first;
        std::string second;
        if (separator != std::string::npos) {
            first = text.substr(0, separator);
            second = text.substr(separator + 2);
        } else {
            const auto space = text.find_first_of(" ,;");
            if (space == std::string::npos) {
                return false;
            }
            first = text.substr(0, space);
            second = text.substr(space + 1);
        }

        try {
            std::size_t consumed = 0;
            minimum = std::stod(first, &consumed);
            maximum = std::stod(second, &consumed);
        } catch (const std::exception&) {
            return false;
        }
        return true;
    }

    /**
     * @brief The parameter list, as an object of name to range.
     *
     * A custom parameter rather than one of the factories, because a CLOP parameter carries three
     * values -- name, lowest, highest -- and every declared parameter shape in this API carries
     * one. An object of name to "low high" is the one form that fits without inventing a nested
     * schema type for a single caller; the alternative, three parallel lists, is a shape a model
     * gets wrong the moment the lists disagree in length.
     */
    Api::Param<Request> parametersParam() {
        return Api::customParam<Request>("parameters", Api::ParamType::StringMap,
            "The parameters to tune, as an object of UCI option name to the range CLOP may search, "
            "e.g. {\"timeShareHalfTime\": \"0 40000\", \"timeShareMin\": \"50 260\"}. Use option "
            "names the engine tool's \"details\" command lists for this engine. Give a range worth "
            "spending games on rather than the engine's full declared range. Sending this replaces "
            "the whole list.",
            [](Request& request, const QaplaTester::Json::JsonValue& value,
                std::vector<std::string>& problems) {
                if (!value.is_object()) {
                    problems.push_back("\"parameters\" must be an object of name to range");
                    return;
                }
                for (const auto& [name, range] : value.as_object()) {
                    auto text = Api::Detail::asText(range);
                    double minimum = 0.0;
                    double maximum = 0.0;
                    if (!text || !parseRange(*text, minimum, maximum)) {
                        problems.push_back("\"" + name +
                            "\" needs a range of two numbers, e.g. \"0 40000\"");
                        continue;
                    }
                    request.parameters.push_back(
                        {.name = name, .minimum = minimum, .maximum = maximum});
                }
            });
    }
} // namespace

void registerClopTools(GuiToolRegistry& registry) {
    Api::defineTool<Request>(registry,
        {.name = "configure_clop",
            .description =
                "Sets up CLOP parameter tuning: one engine's UCI parameters are optimised by "
                "playing it against opponents and fitting a model to the results. Every field is "
                "optional and applied on its own, so pass only what should change -- except "
                "\"engine\" and \"opponents\", which go together because they carry the two roles. "
                "It needs an engine, at least one opponent, at least one parameter with a range, "
                "a time control and an openings file; then start it with the start tool, "
                "type=\"clop\". The run "
                "reports its best estimate as it goes, and writes nothing to any engine -- use the "
                "engine tool to apply the values you decide to keep.",
            .params = {
                Api::stringParam<Request>("engine", &Request::engine,
                    "Catalog name of the engine whose parameters are tuned."),
                Api::stringListParam<Request>("opponents", &Request::opponents,
                    "Catalog names of the engines it plays against, used in turn."),
                parametersParam(),
                Api::stringParam<Request>("time_control", &Request::timeControl,
                    "The clock both sides play at, e.g. \"20+0.1\" for 20 seconds plus 0.1 per "
                    "move. Required: CLOP has no clock of its own, it plays at whatever the "
                    "engines carry, and a run without one cannot start."),
                Api::stringParam<Request>("openings_file", &Request::openingsFile,
                    "Path to an existing EPD/PGN opening book file on disk."),
                Api::integerParam<Request>("samples", &Request::samples,
                    "How many parameter vectors to sample in total. The run ends when it reaches "
                    "this."),
                Api::integerParam<Request>("games_per_sample", &Request::gamesPerSample,
                    "Games played per sampled parameter vector."),
                Api::integerParam<Request>("warmup_samples", &Request::warmupSamples,
                    "Initial random samples taken before the local fit begins."),
                Api::integerParam<Request>("active_pairs", &Request::maxActivePairs,
                    "How many sample pairs may be unfinished at once."),
                Api::integerParam<Request>("concurrency", &Request::concurrency,
                    "Games to run in parallel. The one setting that also takes effect on a run "
                    "that is already going."),
                Api::integerParam<Request>("seed", &Request::seed,
                    "Random seed for opening selection. Same seed, same openings."),
                Api::numberParam<Request>("h", &Request::h,
                    "Locality factor H for the CLOP weighting."),
                Api::numberParam<Request>("prior_variance", &Request::priorVariance,
                    "Gaussian prior variance for the regressions.")},
            .invoke = [](const Request& request) { return Actions::configureClop(request); },
            // Configured and read from outside only: CLOP has no tab, so a chat that set it up
            // would leave the user with numbers they cannot see anywhere in the window.
            .remoteOnly = true});
}

} // namespace QaplaLlm
