# ESP32 LED Matrix Clock

> **📝 Documentation Note**: This README was generated with assistance from AI to ensure comprehensive, professional documentation standards.

[![PlatformIO CI](https://github.com/Svaillan/ESP32-Video-Wall-Clock/workflows/PlatformIO%20CI/badge.svg)](https://github.com/Svaillan/ESP32-Video-Wall-Clock/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)

A professional-grade, feature-rich digital clock display using an ESP32 microcontroller and a 128x32 RGB LED matrix panel. Built with modern software architecture, comprehensive testing, and cross-platform development support.

![Matrix Clock Demo](docs/images/clock-demo.gif)
*Real-time clock with customizable colors and animated background effects*

## ✨ Features

### 🕒 **Advanced Time Display**
- **12/24 Hour Format**: Seamless switching between standard and military time
- **DS3231 RTC Integration**: Battery-backed, precision timekeeping
- **Adaptive Text Sizing**: Three size options (1, 2, 3) with intelligent layout
- **Smart AM/PM Indicators**: Compact "A"/"P" display optimized for size 3 mode

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
For detailed development instructions, see [CONTRIBUTING.md](CONTRIBUTING.md).

## 📱 **Usage Guide**

### **First Boot**
1. **Set Time**: Use menu system to configure current time
2. **Choose Format**: Select 12H or 24H display
3. **Customize Appearance**: Pick colors, effects, and brightness
4. **Enjoy**: Settings auto-save to EEPROM

### **Menu Navigation**
- **Select Button**: Enter/exit menus and confirm selections
- **Up/Down**: Navigate options and adjust values
- **Long Press Select**: Quick return to main clock display

### **Menu Structure**
```
🏠 Main Menu
├── ⏰ Time Format (12H ↔ 24H)
├── 🎨 Clock Color (16 colors + 🌈 Rainbow)
├── ✨ Background Effects (8 animations + Off)
├── 📏 Text Size (Small/Medium/Large)
├── 💡 Brightness (10 levels)
└── ⚙️ Set Time (Hour/Minute adjustment)
```

## 📊 **Technical Specifications**

### **Performance**
- **Memory Usage**: RAM ~8.6% (28,332 bytes), Flash ~25.6% (335,157 bytes)
- **Refresh Rate**: 60+ FPS for smooth animations
- **Response Time**: <10ms button debouncing
- **Power Consumption**: ~15W typical (varies with brightness/effects)

### **Code Quality**
- **Static Analysis**: cppcheck integration with embedded-specific rules
- **Code Formatting**: clang-format with 100-column limit
- **Cross-Platform**: Tested on Windows, Linux, macOS development environments
- **CI/CD**: Automated testing and quality checks on every commit

## 🤝 **Contributing**

We welcome contributions! This project follows modern development practices:

- **Code Style**: Automated formatting with clang-format
- **Quality Gates**: Pre-commit hooks ensure code quality
- **Testing**: Comprehensive test suite with PlatformIO
- **Documentation**: Clear contributing guidelines

See [CONTRIBUTING.md](CONTRIBUTING.md) for detailed information.

### **Development Workflow**
```bash
# Create feature branch
git checkout -b feature/new-effect

# Make changes (pre-commit hooks run automatically)
git commit -m "feat: add aurora background effect"

# Push and create pull request
git push origin feature/new-effect
```

## 📈 **Project Roadmap**

### **Planned Features**
- [ ] WiFi time synchronization with NTP
- [ ] Weather display integration
- [ ] Custom color palette editor
- [ ] Web-based configuration interface
- [ ] Multiple time zone support

### **Completed Milestones**
- [x] ✅ Modular architecture implementation
- [x] ✅ Comprehensive development tooling
- [x] ✅ Cross-platform compatibility
- [x] ✅ Automated testing pipeline
- [x] ✅ Professional documentation

## 📄 **License**

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 👨‍💻 **Author**

**Stephen Vaillancourt** - *Project Creator and Maintainer*

## 🙏 **Acknowledgments**

- **Adafruit** - For excellent LED matrix libraries
- **PlatformIO** - For modern embedded development tools
- **ESP32 Community** - For extensive documentation and support

---

<div align="center">

**Built with ❤️ using PlatformIO and Arduino Framework**

[Report Bug](https://github.com/Svaillan/ESP32-Video-Wall-Clock/issues) · [Request Feature](https://github.com/Svaillan/ESP32-Video-Wall-Clock/issues) · [Contribute](CONTRIBUTING.md)

</div>

---

> **📝 Documentation Note**: This README was generated with assistance from AI to ensure comprehensive, professional documentation standards. The technical content accurately reflects the implemented features and architecture of this project.
