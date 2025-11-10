cmake_minimum_required(VERSION 3.20)
project(QaplaChess)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
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

add_subdirectory(extern/glfw)

# --------------------------------
# ImGui
# --------------------------------
add_library(imgui STATIC
  extern/imgui/imgui.cpp
  extern/imgui/imgui_draw.cpp
  extern/imgui/imgui_tables.cpp
  extern/imgui/imgui_widgets.cpp
  extern/imgui/backends/imgui_impl_glfw.cpp
  extern/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
  extern/imgui
  extern/imgui/backends
  extern/glfw/include
  )
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
)

target_link_libraries(qapla PRIVATE glfw imgui)

# --------------------------------
# Plattformabhängige Links
# --------------------------------
if(WIN32)
  target_link_libraries(qapla PRIVATE opengl32)
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
