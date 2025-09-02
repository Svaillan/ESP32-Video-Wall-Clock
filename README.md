# ESP32 LED Matrix Clock

> **📝 Documentation Note**: This README was generated with assistance from AI to ensure comprehensive, professional documentation standards.

[![PlatformIO CI](https://github.com/Svaillan/ESP32-Video-Wall-Clock/workflows/PlatformIO%20CI/badge.svg)](https://github.com/Svaillan/ESP32-Video-Wall-Clock/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)

A professional-grade, feature-rich digital clock display using an ESP32 microcontroller and a 128x32 RGB LED matrix panel. Built with modern software architecture, comprehensive testing, and cross-platform development support.

*Real-time clock with customizable colors and animated background effects*

## ✨ Features

### 🕒 **Advanced Time & Synchronization**

- **12/24 Hour Format**: Seamless switching between standard and military time
- **DS3231 RTC Integration**: Battery-backed, precision timekeeping
- **NTP (Network Time Protocol) Sync**: Automatic and manual network time synchronization, with non-blocking operation and user feedback
- **Timezone & DST Support**: Dynamic timezone selection, UTC offset, and daylight saving time awareness
- **OTA (Over-the-Air) Updates**: Menu-driven firmware updates with progress display and safe handling
- **WiFi Configuration**: Menu-based WiFi setup, credentials management, and enable/disable logic

### 🎨 **Rich Visual Customization**

- **16+ Clock Colors**: Full spectrum including Red, Green, Blue, Yellow, Cyan, Magenta, White, Orange, Purple, Pink, Lime, Teal, Indigo, Gold, Silver
- **Rainbow Mode**: Smooth animated color cycling
- **10-Level Brightness Control**: Precise adjustment for any environment
- **Separate Effect Brightness**: Independent control for background animations

### 🎆 **Dynamic Background Effects**

- **Confetti**: Colorful particle celebration
- **Matrix Rain**: Green digital cascade (Acid Rain)
- **Weather Effects**: Realistic rain and torrent simulations
- **Cosmic Display**: Twinkling stars with natural variation
- **Sparkles**: Glittering light show
- **Fireworks**: Physics-based explosion animations
- **Smart Masking**: Effects automatically avoid text areas

### 📡 **Messaging System**

- **HTTP API**: RESTful endpoint for sending scrolling messages
- **Password Protection**: Configurable API authentication for security
- **Priority Levels**: Normal, high, and urgent message priorities
- **Queue Management**: Dual-queue system (6 pending + 8 display slots)
- **Rate Limiting**: Configurable message frequency protection
- **Memory Monitoring**: Automatic low-memory protection
- **Length Validation**: Configurable maximum message length (500 chars)
- **Real-time Display**: Smooth right-to-left scrolling with configurable speed
- **High Priority Interruption**: Urgent messages interrupt current display
- **Enter Key Cancellation**: Quick manual message dismissal

### 🧩 **Modular & Maintainable Architecture**

- **Fully Modular Libraries**: All major features separated into reusable libraries (AppStateManager, ButtonManager, ClockDisplay, EffectsEngine, MatrixDisplayManager, MenuSystem, SettingsManager, SystemManager, TimeManager, WiFiManager, WiFiInfoDisplay)
- **Comprehensive Settings Management**: Persistent storage for all preferences, including time, effects, and connectivity
- **Pre-commit Hooks & CI**: Automated code quality checks, formatting, and continuous integration

### 🎮 **Intuitive Interface**

- **Live Preview**: Real-time effect and color preview while browsing
- **Responsive Controls**: Optimized button timing and debouncing
- **Visual Feedback**: Clear menu indicators and selection markers
- **Persistent Settings**: EEPROM storage retains all preferences

## 🏗️ **Modern Architecture**

This project features a **modular, maintainable architecture** with separated concerns:

```
├── AppStateManager/     # Centralized state management
├── ButtonManager/       # Input handling with debouncing
├── ClockDisplay/        # Time rendering and formatting
├── EffectsEngine/       # Background animation system
├── MatrixDisplayManager/ # Hardware abstraction layer
├── MenuSystem/          # Navigation and configuration
└── SettingsManager/     # Persistent configuration storage
```

### 🔧 **Development Features**

- **Automated Code Quality**: Pre-commit hooks with clang-format and cppcheck
- **Cross-Platform Support**: Windows, Linux, macOS development environments
- **Comprehensive Testing**: Automated CI/CD pipeline with GitHub Actions
- **Professional Standards**: Industry best practices and coding standards

## 🛠️ **Hardware Requirements**

### **Core Components**

| Component | Specification | Purpose |
|-----------|---------------|---------|
| **Microcontroller** | ESP32-D0WD-V3 or compatible | Main processing unit |
| **LED Matrix** | 128x32 RGB HUB75 Panel | Primary display |
| **RTC Module** | DS3231 with battery backup | Accurate timekeeping |
| **Input Controls** | 3x Push buttons | Menu navigation |
| **Power Supply** | 5V, 3-4A recommended | System power |

### **Pin Configuration**

```cpp
// Matrix Interface (HUB75)
R1, G1, B1, R2, G2, B2 → GPIO 25, 26, 27, 14, 12, 13
A, B, C, D (Address)    → GPIO 23, 19, 5, 17
CLK, LAT, OE           → GPIO 16, 4, 15

// I2C (RTC)
SDA, SCL → GPIO 21, 22

// Controls (configurable)
Up, Down, Select → GPIO pins of choice
```

## 🚀 **Quick Start**

### **Prerequisites**

- Python 3.7+ with pip
- Git
- VS Code (recommended) or preferred IDE

### **Installation**

```bash
# Clone the repository
git clone https://github.com/Svaillan/ESP32-Video-Wall-Clock.git
cd "ESP32-Video-Wall-Clock"

# Install development tools
pip install platformio pre-commit

# Setup development environment
pre-commit install

# Build and upload
pio run --target upload

# Monitor serial output
pio device monitor
```

### **Development Setup**

````markdown
# ESP32 LED Matrix Clock

[![PlatformIO CI](https://github.com/Svaillan/ESP32-Video-Wall-Clock/workflows/PlatformIO%20CI/badge.svg)](https://github.com/Svaillan/ESP32-Video-Wall-Clock/actions) [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

An ESP32-powered digital clock for a 128x32 RGB LED matrix with customizable visuals, effects, and a REST message API.

Highlights
----------
- 12/24 hour support with DS3231 RTC and optional NTP sync.
- OTA updates and menu-driven WiFi configuration.
- Multiple background effects (fireworks, confetti, matrix rain, etc.) with smart masking to avoid text.
- HTTP message API with password protection, queuing, and priority handling.
- Modular codebase split into reusable libraries for clarity and testing.

Quick start
-----------

Prerequisites: Python 3.7+, PlatformIO, Git

```bash
git clone https://github.com/Svaillan/ESP32-Video-Wall-Clock.git
cd "ESP32-Video-Wall-Clock"
pip install platformio pre-commit
pre-commit install
pio run --target upload
pio device monitor
````

## Message API (example)

Set the password in `credentials/message_config.h`:

```cpp
#define MESSAGE_API_PASSWORD "your_secure_password"
```

Send a message with a bearer token:

```bash
curl -X POST http://<device-ip>/messages \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer your_secure_password" \
  -d '{"text":"Hello","priority":"normal"}'
```

## Documentation and tests

See `CONTRIBUTING.md` for development setup, and the `MESSAGE_SYSTEM_TEST.md` for message-specific test cases.

## License

MIT — see `LICENSE`.

## Author

Stephen Vaillancourt

```
- `high` - Interrupts current display
```
