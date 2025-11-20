# qapla-chess-gui

A modern chess GUI with engine support, written in C++ using OpenGL and ImGui.

## Overview

**Qapla Chess GUI** is a professional-grade chess interface specifically designed for **chess engine developers** and **engine testers**. While it can be used for regular chess play, its primary focus is on providing powerful tools for engine development, testing, and tournament management.

### Key Features

#### Tournament Management
- **Parallel Tournament Execution** - Run multiple tournaments simultaneously with up to 32 parallel games
- **SPRT Testing** (Sequential Probability Ratio Test) - Statistical engine comparison with parallel game execution
- **Resumable & Extendable** - Tournaments can be interrupted and resumed later, or extended with additional rounds
- **Gauntlet & Round-Robin** formats with comprehensive rating tracking
- **Live Monitoring** - Watch any ongoing game via tab-based interface; switch between games in real-time

#### Position Analysis
- **Multi-threaded EPD Analysis** - Analyze positions from EPD files using multiple engines in parallel
- **UCI Engine Testing** - Comprehensive engine compliance and behavior validation
- **Extensive Logging** - Detailed protocol communication and engine behavior tracking

#### Game Management
- **PGN Support** - Load, filter, and analyze PGN game databases
- **Multiple Board Workspaces** - Open and manage multiple games/positions simultaneously via tabs
- **Advanced Filtering** - Search and filter games by various criteria

#### User Experience
- **Integrated Tutorial** - Built-in help system for new users
- **Comprehensive Settings** - All features of the command-line engine tester accessible via GUI
- **Real-time Control** - Adjust concurrency, monitor progress, and control running tournaments on the fly
- **Modern Interface** - Clean, responsive UI with OpenGL rendering
- **Initial Multilingual Support** - Basic localization support (English, Deutsch, Français). Select your preferred language in the Settings; translations are work-in-progress and may be incomplete.

### Architecture

All tournament and testing capabilities are powered by the integrated **[qapla-engine-tester](https://github.com/Mangar2/qapla-engine-tester)** engine, which provides:
- UCI protocol support with full pondering capability
- Flexible opening book formats (PGN, EPD, raw FEN)
- Statistical testing and tournament automation
- Engine behavior analysis and compliance checking

The GUI wraps these powerful command-line features in an intuitive, visual interface while adding multi-board management and real-time game observation capabilities.

## Portable Application

**No installation required!** The compiled executable is a self-contained, portable application. Simply copy the executable file anywhere and run it directly from that location. All fonts and assets are embedded in the binary.

## Supported Platforms

- **Windows** (tested)
- **Linux** (X11 with CMake support)
- **macOS** (with Clang/LLVM)

## Build

This project uses [CMake](https://cmake.org) with [Presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html), [Ninja](https://ninja-build.org), and `clang++`.

### Prerequisites

- CMake ≥ 3.20
- Ninja
- Clang/LLVM (clang++)
- Git

### Clone with Submodules

```bash
git clone --recurse-submodules https://github.com/Mangar2/qapla-chess-gui.git
cd qapla-chess-gui
```

### Build Commands

**Debug Build:**
```bash
cmake --preset default
cmake --build --preset default
```

**Release Build:**
```bash
cmake --preset release
cmake --build --preset release
```

**Diagnostic Engine:**
```bash
cmake --preset default
cmake --build --preset diagnostic
```

## External Dependencies

This project uses the following libraries as Git submodules in the `extern/` directory:

- **[GLFW](https://github.com/glfw/glfw)** - Window and input management (zlib/libpng License)
- **[ImGui](https://github.com/ocornut/imgui)** - Immediate Mode GUI (MIT License)
- **[GLAD](https://github.com/Dav1dde/glad)** - OpenGL Loader (MIT License)
- **[stb](https://github.com/nothings/stb)** - Image loading (Public Domain)
- **[qapla-engine-tester](https://github.com/Mangar2/qapla-engine-tester)** - Engine testing framework

Sources are automatically included via Git submodules and compiled with CMake.

## Fonts

The GUI uses embedded fonts (see `src/font.h` and `src/font.cpp`):

- **Chess Merida Unicode** - Chess piece rendering (Public Domain / Unlicense)
  - Source: `fonts/chess_merida/`
- **Inter Variable** - Modern UI font (SIL Open Font License 1.1)
  - Source: `fonts/inter/`
- **IBM Plex Mono** - Monospace font for code/lists (SIL Open Font License 1.1)
  - Source: `fonts/ibm_plex_mono/`

All fonts are embedded in the binary and require no external installation.

## Graphical Assets

### Background Texture

The project uses the "Dark Wood" texture from [Poly Haven](https://polyhaven.com):

- **License:** CC0 (100% Free)
- No attribution required
- No restrictions for commercial or private use
- Embedded in `src/dark-wood-background.cpp`

From the Poly Haven website:
> "100% Free - Not just free, but CC0, meaning you can use them for absolutely any purpose without restrictions. No paywalls or signup required, simply download what you want and use it immediately without worry."

## License

This project is licensed under the [GNU GPL v3](LICENSE).  

The external libraries and assets used have their own respective licenses:

- **Own Components:** GNU GPL v3
- **GLFW:** zlib/libpng
- **ImGui:** MIT
- **GLAD:** MIT
- **stb:** Public Domain
- **Chess Merida Font:** Public Domain (Unlicense)
- **Inter Font:** SIL Open Font License 1.1
- **IBM Plex Mono:** SIL Open Font License 1.1
- **Dark Wood Texture:** CC0 (Public Domain)
