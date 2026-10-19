# GUI Frontends

The application is written against one toolkit-neutral interface, **QaplaUi**
(`src/ui/qapla-ui.h`), and built once as the object library `qapla-app`. A *frontend* is an
executable that implements QaplaUi on a GUI toolkit and links that library:

| Target | Frontend | Toolkit | Platforms |
|---|---|---|---|
| `qapla` | `src/frontend/imgui` | Dear ImGui on GLFW and OpenGL | Windows, Linux, macOS |
| `QaplaChess.app` | `src/frontend/macos` | AppKit | macOS |

A further frontend — native Win32, say — implements the same declarations and nothing else has
to change.

```
src/
  ui/qapla-ui.h            the interface every window is written against
  app/app-runtime.*        start, frame and shutdown that every frontend shares
  frontend/imgui/          QaplaUi on Dear ImGui, the GLFW main loop, the ImGui test engine
  frontend/macos/          QaplaUi on AppKit, the Cocoa main loop
  ...                      everything else: windows, chatbot, data, LLM tools -- toolkit-free
```

## The model: immediate mode, kept

The application was built as an immediate-mode GUI: every frame the whole interface is described
anew from the application's data, and a call that stands for a control returns what the user did
to it since the last frame. Nothing of that changed. A window still says, each frame:

```cpp
if (ImGuiControls::textButton("Start")) { data.start(); }
QaplaUi::sameLine();
QaplaUi::checkbox("Ponder", config.ponder);
```

and it is the frontend that decides what a button is. For Dear ImGui that is a single call. For a
retained toolkit like AppKit it is a control that lives from frame to frame (see below).

This was chosen over writing the windows a second time per toolkit, as native view controllers:
the application polls its singletons from its draw functions in hundreds of places, and the
chatbot's steps reuse the tab panels inside the chat. Rewriting all of that per toolkit would
have meant three copies of every window, drifting apart. With one description per window, a new
toolkit is one new backend, and the windows behave the same in every frontend because they are
the same code.

## Rules for code outside `src/frontend`

* **No toolkit headers.** `qapla-app` is compiled without the ImGui and GLFW include
  directories; a file that includes `<imgui.h>` does not compile. Use `ui/qapla-ui.h`.
* **Identifiers** follow the ImGui convention the code always used: `"Label"`, `"Label##suffix"`,
  `"Label###id"`, scoped with `QaplaUi::pushId()`. The AppKit frontend matches controls between
  frames by them, so a label that changes every frame must carry a stable `###id`.
* **Coordinates** are window coordinates in points, origin top left. Custom drawing goes through
  `QaplaUi::windowDrawList()`; positions obtained from `getCursorScreenPos()` and
  `getItemRectMin()` are in the same space as the ones drawing takes.
* **Text formatting** is the caller's: `QaplaUi::text(std::format(...))`, never printf formats.
* **Tables** are `ImGuiTable` (the class keeps its historical name): it holds rows, sorting,
  filtering and the current row; `QaplaUi::table()` lays it out. In AppKit that is an
  `NSTableView`, which reads a copy of the rows taken whenever `ImGuiTable::version()` changed.
* The `imgui-*.cpp` file names and `ImGui*` class names are historical too: they are
  toolkit-neutral now. Renaming them was left out deliberately, to keep this branch mergeable with
  the work going on elsewhere.

## What every frontend does

`QaplaApp::AppRuntime` owns everything a frontend must not do differently:

```cpp
QaplaApp::AppRuntime runtime(options.remoteControl, autoRunTests);   // config, windows, LLM, HTTP
while (!runtime.quitRequested()) {
    // ... the toolkit's own frame begin, input ...
    runtime.beginFrame();       // UiThreadWatch::frameBegin, UiUpdateQueue, StaticCallbacks::poll
    runtime.draw();             // the windows through QaplaUi
    // ... the toolkit's render ...
    runtime.endFrame();         // POST /shutdown, autosave, UiThreadWatch::frameEnd
}
runtime.shutdownBeforeWindowCloses();
// ... the toolkit closes its window ...
runtime.shutdownAfterWindowClosed();
```

The order matters beyond tidiness: the HTTP integration tests and the stall measurement
(`UiThreadWatch`, see `CLAUDE.md`) rely on frames coming, on the tool queue being served in
`poll`, and on the shutdown order that makes a session save what it stores. A frontend that keeps
frames coming while its window is minimised (AppKit does) serves the remote control in that time
too. The command line is parsed by `QaplaApp::parseCommandLine()` in every frontend, so
`--config-dir`, `--remote-control` and the rest work the same.

`OsDialogs` asks the frontend for the window its dialogs belong to:
`OsDialogs::setNativeWindowProvider()` — an `HWND` from GLFW in the ImGui frontend on Windows,
the `NSWindow*` in the AppKit one.

## The Dear ImGui frontend

`src/frontend/imgui/imgui-ui-backend.cpp` implements QaplaUi call for call; QaplaUi was cut along
ImGui's own lines for exactly this. The ImGui build therefore looks and behaves as before, and the
ImGui test engine (`src/test-system`) finds every item under its old identifier. The table
renderer, the icon buttons, the command popup and the splitters carry the drawing code that used
to sit in the application.

## The AppKit frontend

`src/frontend/macos` turns the immediate-mode description into views that last.

* **Containers** (`macos-backend.h`) — one per overlay, child region, popup and dialog — lay out
  their items top to bottom with ImGui's rules (`sameLine`, indentation, item spacing, frame
  padding, the 65 % default item width) and keep the **widgets** they made, keyed by the
  identifier of the call. A call in the next frame with the same identifier gets the same widget
  back; its view is updated only where something changed, and what AppKit reported in between
  (a click, typed text, a chosen item) is what the call returns. A widget no call asked for in a
  frame is removed at its end.
* **Native controls**: `NSButton` (push, checkbox, radio), `NSTextField`, `NSTextField` with
  `NSStepper` for numbers, `NSSlider`, `NSProgressIndicator`, `NSPopUpButton` (pull-down) for
  combos, `NSTextView` for the chat input, `NSTableView` for tables, sheets (`NSPanel`) for modal
  dialogs, `NSMenu` for command menus, the menu bar with the standard Edit menu, native tooltips.
  Section headers, list entries, the tab strip and the splitters are small custom views.
* **Drawing** is recorded as `DrawCmd`s and painted with Core Graphics by layer views that sit
  between the controls in the order of the calls, so what was drawn after a control is above it —
  the board, the clocks, the evaluation chart, the toolbar icons. Text items are drawn the same
  way, in the system font; the chess and mono fonts are made from the embedded font data with
  Core Text, without registering them with the system.
* **Input** comes from a local event monitor: clicks, keys and the mouse position as the events
  carry it. Invisible buttons (the board's squares, the icon buttons) are hit-tested against the
  container under the mouse, so a native control above them takes its own clicks.
* **Frames** are driven by a timer on the main run loop in the common modes — 60 per second while
  the user works, 20 while nothing happens — and immediately after any AppKit callback. A tab
  chosen by a click or by `SetSelected` takes effect when its tab bar begins the next frame, as in
  ImGui.
* The window is dark (`NSAppearanceNameDarkAqua`): the application's own drawing — the board's
  coordinates, the clocks, the icons — was designed for a dark background.

## Writing another frontend

1. Add `src/frontend/<name>` and a target in `CMakeLists.txt` that links `qapla-app`, the way
   `QaplaChess` does.
2. Implement every function declared in `src/ui/qapla-ui.h`, plus `QaplaUi::table()`. The AppKit
   frontend is the model for a retained toolkit: its `macos-ui.mm` is independent of AppKit in
   most of what it does (containers, layout, keys, popups, tab state), and its views are the part
   to replace.
3. Write the main loop around `QaplaApp::AppRuntime` as shown above, set
   `OsDialogs::setNativeWindowProvider()`, and parse the command line with
   `QaplaApp::parseCommandLine()`.
4. Check it with the HTTP integration suite (`QAPLA_IT_BINARY=<your binary>`), and look at it
   with `QAPLA_UI_SCRIPT` (below).

## Checking a frontend

* **Integration tests** run against any frontend:
  `QAPLA_IT_BINARY=build/default/QaplaChess.app/Contents/MacOS/QaplaChess python3 test/integration/test_runner.py --filter 'tournament-*'`.
* **Screenshots without screen recording**: `QAPLA_SCREENSHOT=<file.png>` writes the frame
  `QAPLA_SCREENSHOT_FRAME` (default 60); `QAPLA_SCREENSHOT_QUIT=1` ends the application after it.
  In the macOS frontend, `QAPLA_SCREENSHOT_INTERVAL=<seconds>` writes numbered pictures instead,
  for following a session that a script drives over the remote control.
* **Scripted clicks**: `QAPLA_UI_SCRIPT=<file>` plays one command per line — `wait <s>`,
  `click <x> <y>`, `sclick <x> <y>` (into a sheet), `rightclick`, `move`, `type <text>`,
  `key <enter|tab|escape|up|down|left|right|backspace>`, `shot <file.png>`, `quit` — with
  coordinates in points from the top left of the window. In the macOS frontend the events go
  through AppKit like the user's own. The ImGui frontend reads the same file but takes only its
  screenshots and waits reliably: GLFW feeds the real cursor to ImGui as well, and an injected
  click does not always survive that.
