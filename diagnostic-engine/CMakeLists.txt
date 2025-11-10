cmake_minimum_required(VERSION 3.20)
project(DiagnosticEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build the diagnostic engine executable
add_executable(diagnostic-engine
    diagnostic-engine.cpp
)

# Set output directory
set_target_properties(diagnostic-engine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(diagnostic-engine PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# Installation
install(TARGETS diagnostic-engine
    RUNTIME DESTINATION bin
)
