# Chatbot Step Pattern for Multiple Targets

This document describes the pattern for creating chatbot steps that can work with different backend types (tournaments, SPRT, EPD, interactive boards).

## Problem

Some chatbot steps need to configure settings on different types of objects:
- Tournament/SPRT threads: Use `TournamentData::instance()` or `SprtTournamentData::instance()`
- Board thread: Uses specific `InteractiveBoardWindow` instance by ID
- EPD thread: Uses `EpdData` instance

We want to reuse existing steps (like `ChatbotStepGlobalSettings`) across different chatbot threads without duplicating code.

## Solution: Callback-Based Provider Pattern

Steps accept an optional `std::function` callback that provides access to the target object. If no callback is provided, the step uses default behavior (singleton instances for tournaments).

### Example: ChatbotStepGlobalSettings

#### 1. Define the Provider Type

```cpp
class ChatbotStepGlobalSettings : public ChatbotStep {
public:
    using SettingsProvider = std::function<ImGuiEngineGlobalSettings*()>;
    
    // Default constructor for tournaments
    explicit ChatbotStepGlobalSettings(EngineSelectContext type = EngineSelectContext::Standard);
    
    // Constructor with custom provider for boards
    explicit ChatbotStepGlobalSettings(SettingsProvider provider);
```

#### 2. Implement the Provider Logic

```cpp
ImGuiEngineGlobalSettings* ChatbotStepGlobalSettings::getGlobalSettings() {
    // Use custom provider if provided
    if (customProvider_) {
        return customProvider_();
    }
    
    // Otherwise use default singleton-based logic
    if (type_ == EngineSelectContext::SPRT) {
        return &SprtTournamentData::instance().getGlobalSettings();
    }
    return &TournamentData::instance().getGlobalSettings();
}
```

#### 3. Handle Nullptr (Target No Longer Exists)

```cpp
std::string ChatbotStepGlobalSettings::draw() {
    auto* globalSettings = getGlobalSettings();
    
    // Check if target still exists (e.g., board not closed)
    if (globalSettings == nullptr) {
        QaplaWindows::ImGuiControls::textWrapped("Error: Target no longer exists.");
        finished_ = true;
        return "stop";
    }
    
    // Use globalSettings->... instead of globalSettings....
    globalSettings->drawGlobalSettings({}, options);
    // ...
}
```

## Usage in ChatbotBoard

When adding steps in `ChatbotBoard::start()`:

```cpp
void ChatbotBoard::start() {
    steps_.clear();
    // ... reset state ...
    
    steps_.push_back(std::make_unique<ChatbotStepBoardSelect>());
    
    // Add global settings step with lambda provider
    auto globalSettingsProvider = [this]() -> ImGuiEngineGlobalSettings* {
        if (!boardId_.has_value()) {
            return nullptr;
        }
        auto* board = InteractiveBoardWindow::getBoard(*boardId_);
        if (board == nullptr) {
            return nullptr;
        }
        return &board->getGlobalSettings();
    };
    steps_.push_back(std::make_unique<ChatbotStepGlobalSettings>(globalSettingsProvider));
}
```

## Key Points

1. **Callback returns pointer**: Use `T*` instead of `T&` to allow nullptr when target is destroyed
2. **Check for nullptr**: Always check the result before using it
3. **Capture by `[this]`**: The lambda needs access to `boardId_` member
4. **Return `"stop"`**: When target is gone, finish the step and stop the thread
5. **Use arrow operator**: Change from `object.method()` to `pointer->method()`

## Extending to Other Steps

To make any existing step work with boards:

1. Add a callback-based constructor
2. Change internal getters from returning `T&` to `T*`
3. Check for nullptr in `draw()`
4. Use arrow operator instead of dot operator
5. Create lambda provider in `ChatbotBoard::start()`

### Example for Future Engine Selection Step

```cpp
// In chatbot-step-select-engines.h
class ChatbotStepSelectEngines : public ChatbotStep {
public:
    using EngineSelectProvider = std::function<ImGuiEngineSelect*()>;
    
    explicit ChatbotStepSelectEngines(EngineSelectContext type = EngineSelectContext::Standard);
    explicit ChatbotStepSelectEngines(EngineSelectProvider provider);
    // ...
};

// In chatbot-board.cpp
auto engineSelectProvider = [this]() -> ImGuiEngineSelect* {
    if (!boardId_.has_value()) return nullptr;
    auto* board = InteractiveBoardWindow::getBoard(*boardId_);
    return board != nullptr ? &board->getEngineSelect() : nullptr;
};
steps_.push_back(std::make_unique<ChatbotStepSelectEngines>(engineSelectProvider));
```

## Benefits

- **Code Reuse**: Same step class works for tournaments, SPRT, EPD, and boards
- **Type Safety**: Callback signature enforces correct return type
- **Null Safety**: Explicit nullptr checks prevent crashes if board is closed
- **Separation of Concerns**: Steps remain UI-only, threads handle backend wiring
- **Extensible**: Easy to add new target types without modifying step classes
