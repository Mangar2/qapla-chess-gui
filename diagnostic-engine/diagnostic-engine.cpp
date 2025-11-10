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
#include <unistd.h>

std::ofstream logFile;
std::string logFileName;

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
    std::string timestamp = getTimestamp();
    std::string logMsg = "[" + timestamp + "] " + type + ": " + message + "\n";
    
    if (logFile.is_open()) {
        logFile << logMsg;
        logFile.flush();
    }
    
    // Also write to stderr for debugging
    std::cerr << logMsg;
}

void signalHandler(int signum) {
    std::ostringstream oss;
    oss << "Received signal " << signum << " (";
    switch(signum) {
        case SIGTERM: oss << "SIGTERM"; break;
        case SIGINT:  oss << "SIGINT"; break;
        case SIGKILL: oss << "SIGKILL"; break;
        case SIGHUP:  oss << "SIGHUP"; break;
        case SIGPIPE: oss << "SIGPIPE"; break;
        case SIGABRT: oss << "SIGABRT"; break;
        case SIGSEGV: oss << "SIGSEGV"; break;
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
    } else if (signum == SIGPIPE) {
        log("SYSTEM", "Ignoring SIGPIPE and continuing");
        return;
    } else {
        if (logFile.is_open()) {
            logFile.close();
        }
        exit(signum);
    }
}

void setupSignalHandlers() {
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGHUP, signalHandler);
    signal(SIGPIPE, signalHandler);
    signal(SIGABRT, signalHandler);
    signal(SIGSEGV, signalHandler);
}

void sendOutput(const std::string& message) {
    log("OUTPUT", message);
    std::cout << message << std::endl;
    std::cout.flush();
}

int main(int argc, char* argv[]) {
    // Create log file with timestamp including milliseconds
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
        return 1;
    }
    
    log("SYSTEM", "Diagnostic UCI Engine started, PID=" + std::to_string(getpid()));
    log("SYSTEM", "Log file: " + logFileName);
    
    setupSignalHandlers();
    
    std::string line;
    int commandCount = 0;
    
    log("SYSTEM", "Starting command loop, waiting for input on stdin...");
    
    while (true) {
        // Check stdin state BEFORE reading
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
        log("INPUT", "#" + std::to_string(commandCount) + " '" + line + "'");
        
        // Check stdin state after reading
        if (std::cin.eof()) {
            log("SYSTEM", "EOF detected on stdin after reading command");
        }
        if (std::cin.fail()) {
            log("SYSTEM", "FAIL bit set on stdin after reading command");
        }
        if (std::cin.bad()) {
            log("SYSTEM", "BAD bit set on stdin after reading command");
        }
        
        if (line == "uci") {
            sendOutput("id name Diagnostic Engine 1.0");
            sendOutput("id author Qapla Chess GUI Team");
            sendOutput("option name Ponder type check default false");
            sendOutput("option name Hash type spin default 128 min 1 max 4096");
            sendOutput("uciok");
        }
        else if (line == "isready") {
            sendOutput("readyok");
        }
        else if (line == "ucinewgame") {
            log("GAME", "New game started");
            // Just acknowledge, don't crash
        }
        else if (line.substr(0, 8) == "position") {
            log("GAME", "Position set: " + line);
        }
        else if (line.substr(0, 9) == "setoption") {
            log("OPTION", line);
        }
        else if (line.substr(0, 2) == "go") {
            log("SEARCH", "Search command: " + line);
            // Simple response - always suggest e2e4
            sendOutput("info depth 1 score cp 50 nodes 1 nps 1000 time 1");
            sendOutput("bestmove e2e4");
        }
        else if (line == "quit") {
            log("SYSTEM", "Quit command received, shutting down gracefully");
            break;
        }
        else if (line == "stop") {
            log("SEARCH", "Stop command received");
            sendOutput("bestmove e2e4");
        }
        else if (!line.empty()) {
            log("WARNING", "Unknown command: " + line);
        }
    }
    
    // Log WHY the loop ended
    log("SYSTEM", "Command loop ended!");
    log("SYSTEM", "stdin.eof() = " + std::string(std::cin.eof() ? "true" : "false"));
    log("SYSTEM", "stdin.fail() = " + std::string(std::cin.fail() ? "true" : "false"));
    log("SYSTEM", "stdin.bad() = " + std::string(std::cin.bad() ? "true" : "false"));
    log("SYSTEM", "Total commands processed: " + std::to_string(commandCount));
    log("SYSTEM", "Engine exiting normally with exit(0)");
    
    if (logFile.is_open()) {
        logFile.close();
    }
    
    return 0;
}
