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

#include "epd-manager.h"
#include "engine-worker-factory.h"
#include "game-manager.h"
#include "game-manager-pool.h"
#include "game-state.h"
#include "string-helper.h"

std::string EpdManager::generateHeaderLine() const {
    std::ostringstream header;
    auto formatEngineName = [](const std::string& name) -> std::string {
        constexpr size_t totalWidth = 25;
        size_t len = name.length();
        if (len > totalWidth) {
            return "..." + name.substr(len - (totalWidth - 3));
        }
        size_t padding = totalWidth - static_cast<uint32_t>(name.length());
        size_t leftPad = padding / 2;
        size_t rightPad = padding - leftPad;
        return std::string(leftPad, ' ') + name + std::string(rightPad, ' ');
        };
    header << std::setw(20) << std::left << "TestId";
    auto results = getResultsCopy();
    for (const auto& result : results) {
        if (result.testSetName == epdFileName_) {
            header << "|" << formatEngineName(result.engineName);
        }
    }
    
    return header.str();
}

void EpdManager::logHeaderLine() const {
    auto header = generateHeaderLine();
    Logger::testLogger().log(header, TraceLevel::result);
}

static void formatInlineResult(std::ostream& os, const EpdTestCase& test) {
    os << "|" << std::setw(8) << std::right << (test.correct ? QaplaHelpers::formatMs(test.correctAtTimeInMs, 2) : "-")
        << ", D:" << std::setw(3) << std::right << (test.correct ? std::to_string(test.correctAtDepth) : "-")
        << ", M: " << std::setw(5) << std::left << test.playedMove;
}

std::string EpdManager::generateResultLine(const EpdTestCase& current, const TestResults& results) const {
    std::ostringstream line;
    line << std::setw(20) << std::left << current.id;
    for (const auto& result : results) {
        const auto it = std::find_if(result.result.begin(), result.result.end(), [&](const EpdTestCase& t) {
            return t.id == current.id;
            });
        if (it != result.result.end()) {
            formatInlineResult(line, *it);
        }
        else {
            line << "|" << std::setw(23) << "-";
        }
    }

    line << "| BM: ";
    for (const auto& bm : current.bestMoves) {
        line << bm << " ";
    }
    return line.str();
}

void EpdManager::logResultLine(const EpdTestCase& current) const {
	auto results = getResultsCopy();
    auto line = generateResultLine(current, results);
	Logger::testLogger().log(line, TraceLevel::result);
}

void EpdManager::saveResults(std::ostream& os) const {
    auto results = getResultsCopy();
    if (results.empty()) {
        return;
    }
    os << generateHeaderLine() << std::endl;
    for (const auto& testCase : testsRead_) {
        os << generateResultLine(testCase, results) << std::endl;
    }    
}

inline std::ostream& operator<<(std::ostream& os, const EpdTestCase& test) {

    formatInlineResult(os, test);

    os << " | BM: ";
    for (const auto& bm : test.bestMoves) {
        os << bm << " ";
    }
    return os;
}

void EpdManager::initializeTestCases(uint64_t maxTimeInS, uint64_t minTimeInS, uint32_t seenPlies) {
    if (!reader_) {
        throw std::runtime_error("EpdReader must be initialized before loading test cases.");
    }

    reader_->reset();
    testInstances_.clear();

    while (true) {
        auto testCase = nextTestCaseFromReader();
        if (!testCase) {
            break;
        }
		testCase->maxTimeInS = maxTimeInS;
		testCase->minTimeInS = minTimeInS;
		testCase->seenPlies = static_cast<int>(seenPlies);
        testsRead_.push_back(std::move(*testCase));
    }
}

void EpdManager::initialize(const std::string& filepath, 
    uint64_t maxTimeInS, uint64_t minTimeInS, uint32_t seenPlies)
{
	epdFileName_ = filepath;
    bool sameFile = reader_ && reader_->getFilePath() == filepath;
    if (!sameFile) {
        reader_ = std::make_unique<EpdReader>(filepath);
    }

    initializeTestCases(maxTimeInS, minTimeInS, seenPlies);
    tc_.setMoveTime(maxTimeInS * 1000);
}

void EpdManager::clear() {
    if (reader_) {
        reader_->reset();
    }
    testInstances_.clear();
}

void EpdManager::schedule(const EngineConfig& engineConfig) {
    EpdTestResult test;
	test.tc_ = tc_;
    test.engineName = engineConfig.getName();
    test.result = testsRead_;
	test.testSetName = epdFileName_;
	auto newTest = std::make_shared<EpdTest>();
	newTest->initialize(test);
	testInstances_.emplace_back(newTest);
    newTest->setTestResultCallback([this]([[maybe_unused]] EpdTest* test, size_t first, size_t last) {
        auto results = getResultsCopy();
        for (size_t i = first; i < last && i < testsRead_.size(); ++i) {
            const auto& current = testsRead_[i];
            auto line = generateResultLine(current, results);
            Logger::testLogger().log(line, TraceLevel::result);
        }
    });
    logHeaderLine();
    newTest->schedule(newTest, engineConfig);
}

std::optional<EpdTestCase> EpdManager::nextTestCaseFromReader() {
    if (!reader_) {
        return std::nullopt;
    }

    auto entryOpt = reader_->next();
    if (!entryOpt) {
        return std::nullopt;
    }

    const auto& entry = *entryOpt;
    EpdTestCase testCase;
    testCase.fen = entry.fen;
    testCase.original = entry;

    auto it = entry.operations.find("id");
    if (it != entry.operations.end() && !it->second.empty()) {
        testCase.id = it->second.front();
    }

    auto bmIt = entry.operations.find("bm");
    if (bmIt != entry.operations.end()) {
        testCase.bestMoves = bmIt->second;
    }

    return testCase;
}

double EpdManager::getSuccessRate() const {
	int totalTests = 0;
	int correctTests = 0;
	for (const auto& instance : testInstances_) {
		for (const auto& test : instance->getResultsCopy().result) {
			++totalTests;
			if (test.correct) {
				++correctTests;
			}
		}
	}
	return totalTests > 0 ? static_cast<double>(correctTests) / totalTests : 0.0;
}
