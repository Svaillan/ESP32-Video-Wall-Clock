# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Comprehensive pre-commit hooks for code quality
- Cross-platform development environment setup
- Modular architecture with separated concerns
- Enhanced button responsiveness
- Static code analysis with cppcheck
- Automated code formatting with clang-format

### Changed
- Refactored from clock-specific to generic terminology for future extensibility
- Improved button timing for better user experience
- Enhanced effects engine with natural star twinkling and cascading shooting stars

### Fixed
- Set clock functionality that was broken after modularization
- Natural star twinkling behavior (was too robotic)
- Text area detection with proper bounding box calculation
- Array bounds checking in time display functions

## [2.0.0] - 2025-08-31

### Added
- Complete modular architecture with 7 separate libraries
- AppStateManager for centralized state management
- ButtonManager for input handling with debouncing
- ClockDisplay for time rendering and formatting
- EffectsEngine for background animations
- MatrixDisplayManager for display operations
- MenuSystem for navigation and settings
- SettingsManager for persistent configuration

### Changed
- Completely restructured codebase for maintainability
- Separated concerns into logical modules
- Improved code organization and readability

### Migration Notes
- This is a breaking change from the monolithic v1.x architecture
- All functionality has been preserved but reorganized
- Configuration and settings remain compatible

## [1.0.0] - Initial Release

### Added
- Basic ESP32 LED matrix clock functionality
- Multiple color options and background effects
- RTC integration for accurate timekeeping
- Simple menu system for configuration
- 12/24 hour time format support

---

## Categories Used

- **Added** for new features
- **Changed** for changes in existing functionality
- **Deprecated** for soon-to-be removed features
- **Removed** for now removed features
- **Fixed** for any bug fixes
- **Security** in case of vulnerabilities
