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

if(NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
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
# qapla-engine-tester Sources
# --------------------------------
file(GLOB_RECURSE QAPLA_TESTER_SOURCES CONFIGURE_DEPENDS
    extern/qapla-engine-tester/src/*.cpp
)
# qapla-engine-tester.cpp entfernen (hat main())
list(FILTER QAPLA_TESTER_SOURCES EXCLUDE REGEX ".*qapla-engine-tester\\.cpp$")

# --------------------------------
# Eigene Quelldateien
# --------------------------------
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.c
    i18n/*.cpp
)

# --------------------------------
# Executable
# --------------------------------
add_executable(qapla WIN32 ${PROJECT_SOURCES} ${QAPLA_TESTER_SOURCES})

# --------------------------------
# Includes & Libs
# --------------------------------
target_include_directories(qapla PRIVATE
  extern/glad/include
  extern/qapla-engine-tester/src
  src
  i18n
)

target_link_libraries(qapla PRIVATE glfw imgui)

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
# Diagnostic Engine (separate executable)
# --------------------------------
add_subdirectory(diagnostic-engine)
