/**
 * @license
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Diagnostic UCI Engine - Logs all inputs, outputs, and signals
 * Used to diagnose Linux-specific tournament start crashes
 */

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <csignal>
#include <ctime>
#include <random>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <thread>

// Platform-specific includes
#ifdef _WIN32
    #include <process.h>
    #define getpid _getpid
#else
    #include <unistd.h>
#endif

// Include GameState for chess position handling
#include "game-state.h"

using namespace QaplaTester;

// =============================================================================
// Engine Mode Enumeration
// =============================================================================

enum class EngineMode {
    LOG,        // Full logging and UCI functionality
    NOINIT,     // Ignore all input except quit, no logging
    LOOP,       // Infinite loop on isready
    LOSSONTIME  // Progressively waste time until time loss
};

// =============================================================================
// Global Variables
// =============================================================================

std::ofstream logFile;
std::string logFileName;
EngineMode engineMode = EngineMode::LOG;
std::unique_ptr<GameState> gameState;
std::mt19937 rng(std::random_device{}());
int moveCounter = 0; // Counter for LOSSONTIME mode

// =============================================================================
// Utility Functions
// =============================================================================

std::string getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto timer = std::chrono::system_clock::to_time_t(now);
    std::tm bt = *std::localtime(&timer);
    
    std::ostringstream oss;
    oss << std::put_time(&bt, "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void log(const std::string& type, const std::string& message) {
    if (engineMode != EngineMode::LOG) {
        return; // No logging in NOINIT or LOSSONTIME mode
    }
    
    std::string timestamp = getTimestamp();
    std::string logMsg = "[" + timestamp + "] " + type + ": " + message + "\n";
    
    if (logFile.is_open()) {
        logFile << logMsg;
        logFile.flush();
    }
    
    // Also write to stderr for debugging
    std::cerr << logMsg;
}

void sendOutput(const std::string& message) {
    log("OUTPUT", message);
    std::cout << message << std::endl;
    std::cout.flush();
}

// =============================================================================
// Signal Handling
// =============================================================================

void signalHandler(int signum) {
    std::ostringstream oss;
    oss << "Received signal " << signum << " (";
    switch(signum) {
        case SIGTERM: oss << "SIGTERM"; break;
        case SIGINT:  oss << "SIGINT"; break;
        case SIGABRT: oss << "SIGABRT"; break;
        case SIGSEGV: oss << "SIGSEGV"; break;
#ifndef _WIN32
        case SIGKILL: oss << "SIGKILL"; break;
        case SIGHUP:  oss << "SIGHUP"; break;
        case SIGPIPE: oss << "SIGPIPE"; break;
#endif
        default:      oss << "UNKNOWN"; break;
    }
    oss << ")";
    log("SIGNAL", oss.str());
    
    if (signum == SIGTERM || signum == SIGINT) {
        log("SYSTEM", "Graceful shutdown initiated");
        if (logFile.is_open()) {
            logFile.close();
        }
        exit(0);
    }
#ifndef _WIN32
    else if (signum == SIGPIPE) {
        log("SYSTEM", "Ignoring SIGPIPE and continuing");
        return;
    }
#endif
    else {
        if (logFile.is_open()) {
            logFile.close();
        }
        exit(signum);
    }
}

void setupSignalHandlers() {
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGABRT, signalHandler);
    signal(SIGSEGV, signalHandler);
#ifndef _WIN32
    signal(SIGHUP, signalHandler);
    signal(SIGPIPE, signalHandler);
#endif
}

// =============================================================================
// Engine Mode Detection
// =============================================================================

EngineMode detectEngineMode(const std::string& executablePath) {
    std::filesystem::path path(executablePath);
    std::string filename = path.stem().string();
    
    // Convert to lowercase for comparison
    std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
    
    if (filename.find("noinit") != std::string::npos) {
        return EngineMode::NOINIT;
    } else if (filename.find("loop") != std::string::npos) {
        return EngineMode::LOOP;
    } else if (filename.find("lossontime") != std::string::npos) {
        return EngineMode::LOSSONTIME;
    } else {
        return EngineMode::LOG;
    }
}

// =============================================================================
// Logging Initialization
// =============================================================================

bool initializeLogging() {
    if (engineMode != EngineMode::LOG) {
        return true; // No logging needed
    }
    
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto timer = std::chrono::system_clock::to_time_t(now);
    std::tm bt = *std::localtime(&timer);
    
    std::ostringstream logFileNameStream;
    logFileNameStream << "diagnostic-engine-" 
                      << std::put_time(&bt, "%Y-%m-%d_%H-%M-%S")
                      << "-" << std::setfill('0') << std::setw(3) << ms.count()
                      << "-pid" << getpid() << ".log";
    logFileName = logFileNameStream.str();
    
    logFile.open(logFileName, std::ios::out | std::ios::app);
    
    if (!logFile.is_open()) {
        std::cerr << "ERROR: Could not open log file: " << logFileName << std::endl;
        return false;
    }
    
    log("SYSTEM", "Diagnostic UCI Engine started, PID=" + std::to_string(getpid()));
    log("SYSTEM", "Log file: " + logFileName);
    
    return true;
}

// =============================================================================
// UCI Command Handlers
// =============================================================================

void handleUciCommand() {
    sendOutput("id name Diagnostic Engine 1.0");
    sendOutput("id author Qapla Chess GUI Team");
    sendOutput("option name Ponder type check default false");
    sendOutput("option name Hash type spin default 128 min 1 max 4096");
    sendOutput("uciok");
}

void handleIsReadyCommand() {
    if (engineMode == EngineMode::LOOP) {
        log("SEARCH", "Entering infinite loop (LOOP mode)");
        while (true) {
            // Infinite loop - engine hangs here
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    sendOutput("readyok");
}

void handlePositionCommand(const std::string& line) {
    log("GAME", "Position set: " + line);
    
    // Parse: "position startpos" or "position fen <fenstring>" [moves <move1> <move2> ...]
    std::istringstream iss(line);
    std::string token;
    iss >> token; // skip "position"
    
    if (!(iss >> token)) {
        log("WARNING", "Invalid position command: missing position type");
        return;
    }
    std::string fenPart;
    
    if (token == "startpos") {
        gameState->setFen(true);
    } else if (token == "fen") {
        std::string fen;
        // Read FEN parts until we hit "moves" or end of line
        while (iss >> fenPart && fenPart != "moves") {
            if (!fen.empty()) fen += " ";
            fen += fenPart;
        }
        
        if (!fen.empty()) {
            if (!gameState->setFen(false, fen)) {
                log("WARNING", "Invalid FEN: " + fen);
                return;
            }
        } else {
            log("WARNING", "Empty FEN string");
            return;
        }
        
        // If we stopped at "moves", put it back
        if (fenPart == "moves") {
            // Continue to process moves below
        }
    } else {
        log("WARNING", "Unknown position type: " + token);
        return;
    }
    
    // Look for "moves" keyword
    if (fenPart == "moves") {
        token = "moves";
    } else {
        while (iss >> token && token != "moves") {
            // Skip until we find "moves"
        }
    }
    
    if (token == "moves") {
        // Parse and apply moves
        std::string moveStr;
        while (iss >> moveStr) {
            auto move = gameState->stringToMove(moveStr, false);
            if (move.isEmpty()) {
                log("WARNING", "Invalid move: " + moveStr);
                break;
            }
            gameState->doMove(move);
            log("GAME", "Applied move: " + moveStr + " -> " + move.getLAN());
        }
    }
    
    log("GAME", "Current position FEN: " + gameState->getFen());
}

void handleGoCommand(const std::string& line) {
    log("SEARCH", "Search command: " + line);
    
    // LOSSONTIME mode: waste increasing amounts of time
    if (engineMode == EngineMode::LOSSONTIME) {
        moveCounter++;
        
        // Parse go command to extract time information
        std::istringstream iss(line);
        std::string token;
        long long wtime = 0, btime = 0, winc = 0, binc = 0, movetime = 0;
        
        while (iss >> token) {
            if (token == "wtime") {
                iss >> wtime;
            } else if (token == "btime") {
                iss >> btime;
            } else if (token == "winc") {
                iss >> winc;
            } else if (token == "binc") {
                iss >> binc;
            } else if (token == "movetime") {
                iss >> movetime;
            }
        }
        
        // Determine available time for this move
        long long availableTime = 0;
        bool isWhite = gameState->isWhiteToMove();
        
        if (movetime > 0) {
            availableTime = movetime;
        } else if (isWhite && wtime > 0) {
            availableTime = wtime;
        } else if (!isWhite && btime > 0) {
            availableTime = btime;
        }
        
        double sleepPercentage = moveCounter * 0.10;
        long long sleepTime = static_cast<long long>(availableTime * sleepPercentage);
        
        // Sleep for the calculated time
        if (sleepTime > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
        }
    }
    
    // Get all legal moves
    auto legalMoves = gameState->getLegalMoves();
    
    if (legalMoves.empty()) {
        log("SEARCH", "No legal moves available (checkmate or stalemate)");
        sendOutput("info string No legal moves");
        sendOutput("bestmove (none)");
        return;
    }
    
    // Pick a random legal move
    std::uniform_int_distribution<size_t> dist(0, legalMoves.size() - 1);
    size_t randomIndex = dist(rng);
    auto selectedMove = legalMoves[randomIndex];
    
    std::string bestmove = selectedMove.getLAN();
    log("SEARCH", "Randomly selected move " + std::to_string(randomIndex + 1) + 
                  " of " + std::to_string(legalMoves.size()) + ": " + bestmove);
    
    // Send info and bestmove immediately
    sendOutput("info depth 1 score cp 0 nodes " + std::to_string(legalMoves.size()) + 
               " nps 1000 time 1");
    sendOutput("bestmove " + bestmove);
}

// =============================================================================
// Command Processing
// =============================================================================

bool processCommand(const std::string& line, int commandCount) {
    log("INPUT", "#" + std::to_string(commandCount) + " '" + line + "'");
    
    if (line == "uci") {
        handleUciCommand();
    }
    else if (line == "isready") {
        handleIsReadyCommand();
    }
    else if (line == "ucinewgame") {
        log("GAME", "New game started");
        gameState->setFen(true); // Reset to starting position
        moveCounter = 0; // Reset move counter for LOSSONTIME mode
    }
    else if (line.substr(0, 8) == "position") {
        handlePositionCommand(line);
    }
    else if (line.substr(0, 9) == "setoption") {
        log("OPTION", line);
    }
    else if (line.substr(0, 2) == "go") {
        handleGoCommand(line);
    }
    else if (line == "quit") {
        log("SYSTEM", "Quit command received, shutting down gracefully");
        return false; // Signal to exit
    }
    else if (line == "stop") {
        log("SEARCH", "Stop command received");
        // Send a random move as well
        auto legalMoves = gameState->getLegalMoves();
        if (!legalMoves.empty()) {
            std::uniform_int_distribution<size_t> dist(0, legalMoves.size() - 1);
            auto selectedMove = legalMoves[dist(rng)];
            sendOutput("bestmove " + selectedMove.getLAN());
        } else {
            sendOutput("bestmove (none)");
        }
    }
    else if (!line.empty()) {
        log("WARNING", "Unknown command: " + line);
    }
    
    return true; // Continue processing
}

// =============================================================================
// Main Loop - LOG Mode
// =============================================================================

void runLogMode() {
    std::string line;
    int commandCount = 0;
    
    log("SYSTEM", "Starting command loop, waiting for input on stdin...");
    
    while (true) {
        log("SYSTEM", "BEFORE getline: eof=" + std::string(std::cin.eof() ? "true" : "false") + 
                      " fail=" + std::string(std::cin.fail() ? "true" : "false") + 
                      " bad=" + std::string(std::cin.bad() ? "true" : "false"));
        
        if (!std::getline(std::cin, line)) {
            log("SYSTEM", "getline FAILED! eof=" + std::string(std::cin.eof() ? "true" : "false") + 
                          " fail=" + std::string(std::cin.fail() ? "true" : "false") + 
                          " bad=" + std::string(std::cin.bad() ? "true" : "false"));
            break;
        }
        
        commandCount++;
        
        if (std::cin.eof()) {
            log("SYSTEM", "EOF detected on stdin after reading command");
        }
        if (std::cin.fail()) {
            log("SYSTEM", "FAIL bit set on stdin after reading command");
        }
        if (std::cin.bad()) {
            log("SYSTEM", "BAD bit set on stdin after reading command");
        }
        
        if (!processCommand(line, commandCount)) {
            break;
        }
    }
    
    log("SYSTEM", "Command loop ended!");
    log("SYSTEM", "stdin.eof() = " + std::string(std::cin.eof() ? "true" : "false"));
    log("SYSTEM", "stdin.fail() = " + std::string(std::cin.fail() ? "true" : "false"));
    log("SYSTEM", "stdin.bad() = " + std::string(std::cin.bad() ? "true" : "false"));
    log("SYSTEM", "Total commands processed: " + std::to_string(commandCount));
    log("SYSTEM", "Engine exiting normally with exit(0)");
}

// =============================================================================
// Main Loop - NOINIT Mode
// =============================================================================

void runNoInitMode() {
    std::string line;
    
    while (std::getline(std::cin, line)) {
        if (line == "quit") {
            break;
        }
        // Ignore all other input
    }
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main([[maybe_unused]] int argc, char* argv[]) {
    // Detect engine mode from executable name
    engineMode = detectEngineMode(argv[0]);
    
    // Initialize logging if in LOG mode
    if (!initializeLogging()) {
        return 1;
    }
    
    // Initialize GameState after logging is ready
    try {
        gameState = std::make_unique<GameState>();
        gameState->setFen(true); // Start with initial position
        log("SYSTEM", "GameState initialized successfully");
    } catch (const std::exception& e) {
        log("ERROR", std::string("Failed to initialize GameState: ") + e.what());
        return 1;
    } catch (...) {
        log("ERROR", "Failed to initialize GameState: unknown exception");
        return 1;
    }
    
    // Setup signal handlers
    setupSignalHandlers();
    
    // Run appropriate mode
    switch (engineMode) {
        case EngineMode::LOG:
            log("SYSTEM", "Running in LOG mode");
            runLogMode();
            break;
            
        case EngineMode::NOINIT:
            runNoInitMode();
            break;
            
        case EngineMode::LOOP:
            log("SYSTEM", "Running in LOOP mode");
            runLogMode(); // Same as LOG mode, but isready will loop
            break;
            
        case EngineMode::LOSSONTIME:
            runLogMode(); // Same as LOG mode, but go command wastes time
            break;
    }
    
    if (logFile.is_open()) {
        logFile.close();
    }
    
    return 0;
}