cmake_minimum_required(VERSION 3.20)
project(QaplaChess)

if(APPLE)
  set(CMAKE_OBJC_COMPILER /usr/bin/clang)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # Generate compile_commands.json for IntelliSense

# Statisches Linken der C/C++ Runtime auf Windows für portable exe
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  # Verhindere automatisches Linken der dynamischen Runtime
  add_compile_options(-D_MT)
  add_compile_options($<$<CONFIG:Debug>:-D_DEBUG>)
  add_compile_options($<$<CONFIG:Release>:-DNDEBUG>)
endif()

# Debug-Modus für i18n: Sprachdateien werden direkt gelesen und bei fehlenden Übersetzungen aktualisiert
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(QAPLA_DEBUG_I18N)
endif()

if(APPLE)
  # macOS: libc++ is the system C++ runtime and always present.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
elseif(NOT WIN32)
  # Linux: build against libstdc++, the runtime every distribution ships. libc++
  # has to be installed on purpose, and a machine that carries only a 32-bit copy
  # of it fails the link with "libc++.so.1: file in wrong format". libstdc++ is
  # still linked into the binary, because its version is the one that travels
  # badly: an older distribution answers a newer one with "libstdc++.so.6:
  # version GLIBCXX_... not found". libgcc_s stays dynamic on purpose -- linking
  # it in drags GCC 13's unwinder along, which calls _dl_find_object and so
  # demands glibc 2.35, locking out RHEL 9. The library itself is everywhere.
  add_link_options(-static-libstdc++)
endif()

# --------------------------------
# GLFW
# --------------------------------
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "no")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "no")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "no")
set(GLFW_INSTALL OFF CACHE INTERNAL "no")
# Force X11 on Linux to avoid Wayland decoration issues
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "Build support for Wayland" FORCE)
set(GLFW_BUILD_X11 ON CACHE BOOL "Build support for X11" FORCE)

add_subdirectory(extern/glfw)

# --------------------------------
# ImGui
# --------------------------------
option(QAPLA_WITH_TEST_ENGINE "Build with ImGui Test Engine" OFF)

set(IMGUI_SOURCES
  extern/imgui/imgui.cpp
  extern/imgui/imgui_draw.cpp
  extern/imgui/imgui_tables.cpp
  extern/imgui/imgui_widgets.cpp
  extern/imgui/backends/imgui_impl_glfw.cpp
  extern/imgui/backends/imgui_impl_opengl3.cpp
)

if(QAPLA_WITH_TEST_ENGINE)
  list(APPEND IMGUI_SOURCES
    extern/imgui_test_engine/imgui_test_engine/imgui_te_context.cpp
    extern/imgui_test_engine/imgui_test_engine/imgui_te_coroutine.cpp
    extern/imgui_test_engine/imgui_test_engine/imgui_te_engine.cpp
    extern/imgui_test_engine/imgui_test_engine/imgui_te_exporters.cpp
    extern/imgui_test_engine/imgui_test_engine/imgui_te_perftool.cpp
    extern/imgui_test_engine/imgui_test_engine/imgui_te_ui.cpp
    extern/imgui_test_engine/imgui_test_engine/imgui_te_utils.cpp
    extern/imgui_test_engine/imgui_test_engine/imgui_capture_tool.cpp
  )
endif()

add_library(imgui STATIC ${IMGUI_SOURCES})

target_include_directories(imgui PUBLIC
  extern/imgui
  extern/imgui/backends
  extern/glfw/include
)

if(QAPLA_WITH_TEST_ENGINE)
  target_include_directories(imgui PUBLIC extern/imgui_test_engine/imgui_test_engine)
  target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_TEST_ENGINE)
  target_compile_definitions(imgui PUBLIC IMGUI_TEST_ENGINE_ENABLE_COROUTINE_STDTHREAD_IMPL=1)
endif()

target_compile_options(imgui PRIVATE -Wno-all)

# --------------------------------
# cpp-httplib (header-only HTTP client/server)
# --------------------------------
add_library(httplib INTERFACE)
target_include_directories(httplib INTERFACE extern/httplib)
if(NOT WIN32)
  find_package(Threads REQUIRED)
  target_link_libraries(httplib INTERFACE Threads::Threads)
endif()

# --------------------------------
# qapla-engine-tester Sources
# --------------------------------
file(GLOB_RECURSE QAPLA_TESTER_SOURCES CONFIGURE_DEPENDS
    extern/qapla-engine-tester/src/*.cpp
)
# remove qapla-engine-tester.cpp as it is a main file for engine-tester
list(FILTER QAPLA_TESTER_SOURCES EXCLUDE REGEX ".*qapla-engine-tester\\.cpp$")
# Exculde unit test files for normal build
list(FILTER QAPLA_TESTER_SOURCES EXCLUDE REGEX ".*/test-system/unit/.*")

# The qapla executable is not linked against Catch2, so Catch2-based test
# files that live outside test-system/unit (e.g. json/test/*.cpp) must be
# excluded there too; unit-tests keeps the unfiltered QAPLA_TESTER_SOURCES.
set(QAPLA_TESTER_SOURCES_FOR_GUI ${QAPLA_TESTER_SOURCES})
list(FILTER QAPLA_TESTER_SOURCES_FOR_GUI EXCLUDE REGEX ".*/json/test/.*")

# --------------------------------
# i18n-Dateien automatisch generieren (nur bei Release-Builds unter Windows)
# --------------------------------
if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set(I18N_LANGUAGES deu eng fra)
    foreach(lang ${I18N_LANGUAGES})
        add_custom_command(
            OUTPUT 
                ${CMAKE_SOURCE_DIR}/i18n/${lang}-lang.h 
                ${CMAKE_SOURCE_DIR}/i18n/${lang}-lang.cpp
            COMMAND qapla-chess-tools --embed input=${lang}.lang name=${lang}_lang header=${lang}-lang.h source=${lang}-lang.cpp
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/i18n
            DEPENDS ${CMAKE_SOURCE_DIR}/i18n/${lang}.lang
            COMMENT "Embedding ${lang}.lang"
            VERBATIM
        )
    endforeach()
endif()

# --------------------------------
# Eigene Quelldateien
# --------------------------------
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.c
    i18n/*.cpp
)
# Unit-Test-Dateien ausschließen (nur für unit-tests Target)
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*/test-system/unit/.*")

# --------------------------------
# Windows Icon Resource
# --------------------------------
if(WIN32)
    set(RESOURCE_FILES resources.rc)
endif()

# --------------------------------
# Executable
# --------------------------------
add_executable(qapla WIN32 ${PROJECT_SOURCES} ${QAPLA_TESTER_SOURCES_FOR_GUI} ${RESOURCE_FILES})

# --------------------------------
# Includes & Libs
# --------------------------------
target_include_directories(qapla PRIVATE
  extern/glad/include
  extern/qapla-engine-tester/src
  src
  i18n
)

target_link_libraries(qapla PRIVATE glfw imgui httplib)

# --------------------------------
# Plattformabhängige Links
# --------------------------------
if(WIN32)
  target_link_libraries(qapla PRIVATE opengl32)
  
  # Static linkig of the C/C++ runtime for portable exe without dll-dependency
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Explizit statische Runtime Libraries linken
    target_link_libraries(qapla PRIVATE 
      $<$<CONFIG:Debug>:libcmtd>
      $<$<CONFIG:Release>:libcmt>
    )
  elseif(MSVC)
    # MSVC Compiler
    set_property(TARGET qapla PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  endif()
  
elseif(APPLE)
  target_link_libraries(qapla PRIVATE
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
  )
elseif(UNIX)
  find_package(OpenGL REQUIRED)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
  target_include_directories(qapla PRIVATE ${GTK3_INCLUDE_DIRS})
  target_link_libraries(qapla PRIVATE ${OPENGL_LIBRARIES} ${GTK3_LIBRARIES} dl pthread)
endif()

# --------------------------------
# Catch2 (Unit Testing Framework)
# --------------------------------
option(QAPLA_BUILD_UNIT_TESTS "Build unit tests with Catch2" ON)

if(QAPLA_BUILD_UNIT_TESTS)
  add_subdirectory(extern/Catch2)
  
  # Unit test sources for qapla-chess-gui and qapla-engine-tester
  file(GLOB_RECURSE UNIT_TEST_SOURCES CONFIGURE_DEPENDS
    src/test-system/unit/*.cpp
  )
  
  if(UNIT_TEST_SOURCES)
    # GUI sources needed by unit tests
    set(GUI_TEST_DEPENDENCIES
      src/tournament-result-incremental.cpp
      src/config-group-loader.cpp
      # The option table behind --help and the parser. Pure argv in, struct out, no GUI.
      src/command-line.cpp
      src/tournament-config-sections.cpp
      # The file that decides whether stored settings are overwritten. Free of the GUI stack.
      src/autosavable.cpp
      src/callback-manager.cpp
      src/llm/lm-studio-locator.cpp
      src/llm/lm-studio-client.cpp
      src/llm/gui-tool-registry.cpp
      src/llm/gui-tool-engine-management.cpp
      src/llm/gui-tool-tournament.cpp
      # The HTTP front door. It serves GuiToolRegistry and knows nothing else about the GUI, so
      # it links here on its own -- see src/llm/remote-control-server.h.
      src/llm/remote-control-server.cpp
      # The wait bookkeeping, fed with plain values rather than reading the GUI itself, precisely
      # so the concurrent part can be tested without a window -- see src/llm/activity-watch.h.
      src/llm/activity-watch.cpp
      # The external<->internal tool mapper and its shared vocabulary. Both are free of the GUI
      # stack by design (see src/llm/tools/gui-tools.h), so the unit tests can exercise the
      # mapper directly; the actions themselves stay qapla-only.
      src/llm/api/llm-tool-api.cpp
      src/llm/actions/gui-action-types.cpp
      src/llm/llm-chat-controller.cpp
      src/llm/llm-chat-logger.cpp
      src/llm/llm-finetuning-writer.cpp
      src/os-helpers.cpp
      src/ui-thread-watch.cpp
    )

    # os-helpers.cpp leaves the macOS specific parts to os-helpers-apple.cpp
    if(APPLE)
      list(APPEND GUI_TEST_DEPENDENCIES src/os-helpers-apple.cpp)
    endif()

    add_executable(unit-tests ${UNIT_TEST_SOURCES} ${GUI_TEST_DEPENDENCIES})
    
    target_include_directories(unit-tests PRIVATE
      src
      i18n
      extern/qapla-engine-tester/src
    )
    
    target_link_libraries(unit-tests PRIVATE
      Catch2::Catch2WithMain
      httplib
    )
    
    # Link qapla-engine-tester sources (shared logic)
    target_sources(unit-tests PRIVATE ${QAPLA_TESTER_SOURCES})
    
    # Compiler flags matching main executable
    target_compile_features(unit-tests PRIVATE cxx_std_20)
    
    # Enable CTest integration (optional)
    include(CTest)
    include(Catch)
    catch_discover_tests(unit-tests)
  endif()
endif()

# --------------------------------
# Diagnostic Engine (separate executable)
# --------------------------------
add_subdirectory(diagnostic-engine)
