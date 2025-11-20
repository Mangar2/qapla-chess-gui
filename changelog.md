# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

- Corrected display of "wins on time"
- Added horizontal scrollbar in the move list
- Fixed clock display to use the actual time control from the PGN file after loading a PGN game, instead of the GUI's configured time control
- Ensured time control consistency: Reset to GUI settings on new game and update engine handlers on position changes to prevent mismatches between displayed and active game time controls.
- Improved engine display consistency: Show PGN-defined engines (A vs. B) during move scrolling, but switch to currently configured engines during Play mode to reflect active computation.

## [1.0.0] - 2025-11-18

- Initial version
