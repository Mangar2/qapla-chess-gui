cmake_minimum_required(VERSION 3.20)
project(DiagnosticEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Get the qapla-engine-tester source directory
set(QAPLA_TESTER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../extern/qapla-engine-tester/src")

# Build the diagnostic engine executable
add_executable(diagnostic-engine
    diagnostic-engine.cpp
    # GameState and dependencies
    ${QAPLA_TESTER_DIR}/game-state.cpp
    ${QAPLA_TESTER_DIR}/game-record.cpp
    ${QAPLA_TESTER_DIR}/move-record.cpp
    ${QAPLA_TESTER_DIR}/base-logger.cpp
    ${QAPLA_TESTER_DIR}/logger.cpp
    # qapla-engine core
    ${QAPLA_TESTER_DIR}/qapla-engine/board.cpp
    ${QAPLA_TESTER_DIR}/qapla-engine/movegenerator.cpp
    ${QAPLA_TESTER_DIR}/qapla-engine/hashconstants.cpp
    ${QAPLA_TESTER_DIR}/qapla-engine/bitboardmasks.cpp
    ${QAPLA_TESTER_DIR}/qapla-engine/magics.cpp
    ${QAPLA_TESTER_DIR}/qapla-engine/piecesignature.cpp
    ${QAPLA_TESTER_DIR}/qapla-engine/pst.cpp
)

# Include directories for GameState and qapla-engine
target_include_directories(diagnostic-engine PRIVATE
    ${QAPLA_TESTER_DIR}
    ${QAPLA_TESTER_DIR}/../..
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

# Link threading library
if(APPLE)
    target_link_libraries(diagnostic-engine PRIVATE pthread)
elseif(WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(diagnostic-engine PRIVATE Threads::Threads)
else()
    target_link_libraries(diagnostic-engine PRIVATE pthread)
endif()

# Installation
install(TARGETS diagnostic-engine
    RUNTIME DESTINATION bin
)

# Create copies for different diagnostic modes
add_custom_command(TARGET diagnostic-engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:diagnostic-engine>
        ${CMAKE_BINARY_DIR}/bin/diagnostic-engine${CMAKE_EXECUTABLE_SUFFIX}
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:diagnostic-engine>
        ${CMAKE_BINARY_DIR}/bin/diagnostic-engine-lossontime${CMAKE_EXECUTABLE_SUFFIX}
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:diagnostic-engine>
        ${CMAKE_BINARY_DIR}/bin/diagnostic-engine-loop${CMAKE_EXECUTABLE_SUFFIX}
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:diagnostic-engine>
        ${CMAKE_BINARY_DIR}/bin/diagnostic-engine-noinit${CMAKE_EXECUTABLE_SUFFIX}
    COMMENT "Creating diagnostic-engine mode variants (normal, lossontime, loop, noinit)"
)

install(FILES 
    ${CMAKE_BINARY_DIR}/bin/diagnostic-engine${CMAKE_EXECUTABLE_SUFFIX}
    ${CMAKE_BINARY_DIR}/bin/diagnostic-engine-lossontime${CMAKE_EXECUTABLE_SUFFIX}
    ${CMAKE_BINARY_DIR}/bin/diagnostic-engine-loop${CMAKE_EXECUTABLE_SUFFIX}
    ${CMAKE_BINARY_DIR}/bin/diagnostic-engine-noinit${CMAKE_EXECUTABLE_SUFFIX}
    DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
