# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.3.0] - 2025-12-31

### Added
- **Pentanomial SPRT support**: Added pentanomial statistics collection for paired games (WW, WD, WL, DD, LD, LL counters)
- **Multiple Elo calculation models**: Support for normalized, logistic, and bayesian SPRT models
- **Enhanced SPRT table**: Displays all model/pentanomial variants for comparison
- **Model selection UI**: Added dropdown to select SPRT calculation model (normalized/logistic/bayesian)
- **Pentanomial checkbox**: UI control to enable/disable pentanomial statistics (auto-disabled for bayesian model)

### Changed
- **SPRT calculation**: Integrated fastchess SPRT implementation with support for multiple models and pentanomial statistics
- **Tournament persistence**: Pentanomial statistics are now recalculated when loading saved tournaments
- **Monte Carlo test button**: Improved tooltip with detailed explanation and automatic disable when pentanomial mode is active

### Fixed
- **Configuration validation**: Added checks for pentanomial requirements (swapColors must be enabled, minimum 2 games)
- **Model compatibility**: Bayesian model properly limited to trinomial statistics only

## [0.2.0] - 2025-12-28

- Corrected display of "wins on time"
- Added horizontal scrollbar in the move list
- Fixed clock display to use the actual time control from the PGN file after loading a PGN game, instead of the GUI's configured time control
- Ensured time control consistency: Reset to GUI settings on new game and update engine handlers on position changes to prevent mismatches between displayed and active game time controls.
- Improved engine display consistency: Show PGN-defined engines during move scrolling, but switch to currently configured engines during Play mode to reflect active computation.
- Initial multilingual support: Added language selector (English, Deutsch, Français), i18n subsystem and initial translation files. Translations are partial and still work-in-progress.
- Fixed: Table filter stays consistend when adding new lines now (problem in Game-End-Cause Table filter)
- Fixed: Log information in the engine log view is no longer empty after 1000 log lines. Having only the last 1000 log entries when opening the window is normal. 
- Added clear button for tutorials
- Added lots of tooltips to explain any control
- File filtering: Open file dialogs for tournaments and SPRT now only display files with appropriate extensions
- Improved Game-End-Cause Table: Filtering the table no longer causes the window layout to jump
- Added tooltips to game viewer tabs showing detailed information (Round, Game, Engines or Position/Engine)
- Clicking on an active game in the tournament window now opens the corresponding game tab
- Engine selections are now always open to simplify user experience
- Chatbot, first version to create tournaments
- Windows file dialogs can no longer get to the background
- Proper Sqpr-Tournament is finished detection including a snackbar message notifying it. 

## [0.1.0] - 2025-11-18

- Initial version
