# Contributing to ESP32 LED Matrix Clock

Thank you for your interest in contributing to this project! Here are some guidelines to help you get started.

## Development Setup

### Prerequisites
- Python 3.7+ with pip
- Git
- VS Code (recommended) or any text editor

### Getting Started
1. Fork the repository
2. Clone your fork:
   ```bash
   git clone https://github.com/yourusername/ESP32-Video-Wall-Clock.git
   cd "ESP32-Video-Wall-Clock"
   ```

3. Install development tools:
   ```bash
   pip install platformio pre-commit
   pre-commit install
   ```

4. Build and test:
   ```bash
   pio run
   pio test
   ```

## Code Style

This project uses automated code formatting and quality checks:

- **C++ Code**: Formatted with `clang-format` (see `.clang-format`)
- **Static Analysis**: Uses `cppcheck` for code quality
- **Pre-commit Hooks**: Automatically run on commit

### Running Checks Manually
```bash
# Run all pre-commit hooks
pre-commit run --all-files

# Run specific checks
pio check                    # PlatformIO static analysis
clang-format -i src/*.cpp    # Format code
```

## Project Structure

```
├── src/           # Main application code
├── lib/           # Custom libraries (modular components)
├── include/       # Header files
├── test/          # Unit tests
├── platformio.ini # PlatformIO configuration
└── README.md      # Project documentation
```

## Hardware Requirements

- ESP32 development board
- 128x32 RGB LED matrix panel (HUB75 interface)
- DS3231 RTC module
- Push buttons for navigation
- Appropriate power supply

## Making Changes

### Code Guidelines
1. **Modular Design**: Keep code organized in logical libraries
2. **Comments**: Document complex logic and hardware interfaces
3. **Testing**: Add tests for new features when possible
4. **Compatibility**: Ensure changes work across development platforms

### Commit Messages
Use conventional commit format:
```
feat: add new background effect
fix: resolve button debouncing issue
docs: update installation instructions
```

### Pull Request Process
1. Create a feature branch from `main`
2. Make your changes
3. Ensure all tests pass and pre-commit hooks succeed
4. Submit a pull request with a clear description

## Reporting Issues

When reporting bugs, please include:
- Hardware configuration
- PlatformIO version
- Steps to reproduce
- Expected vs actual behavior
- Serial output if applicable

## Questions?

Feel free to open an issue for questions or discussion about the project!
