# ESP32 LED Matrix Clock

A feature-rich digital clock display using an ESP32 microcontroller and a 128x32 RGB LED matrix panel. This project includes customizable colors, background effects, multiple time formats, and an intuitive menu system.

## Features

### Time Display
- **12/24 Hour Format**: Switch between standard and military time
- **Real-Time Clock**: DS3231 RTC module for accurate timekeeping
- **Text Size Options**: Three size settings (1, 2, 3) for optimal visibility
- **Smart AM/PM Display**: Compact "A"/"P" indicators for size 3 in 12-hour mode

### Clock Colors (16 Options)
- Red, Green, Blue, Yellow, Cyan, Magenta, White
- Orange, Purple, Pink, Lime, Teal, Indigo, Gold, Silver
- **Rainbow Mode**: Animated cycling through all colors

### Background Effects (8 Options)
- **Confetti**: Colorful falling particles
- **Acid Rain**: Green digital rain effect
- **Rain**: Blue water-like droplets  
- **Torrent**: Heavy rain simulation
- **Stars**: Twinkling starfield
- **Sparkles**: Glittering light effects
- **Fireworks**: Realistic explosion animations with physics
- **Off**: Clean display without effects

### Menu System
- **Live Preview**: See effects and colors in real-time while browsing
- **Boxed Text**: Clear menu text with background boxes for visibility
- **Current Selection**: Asterisk (*) marks active settings
- **Button Navigation**: Up/Down/Select controls

### Display Settings
- **Brightness Control**: 10 levels of brightness adjustment
- **Effect Brightness**: Separate brightness system for background effects
- **Text Masking**: Effects respect clock text boundaries
- **EEPROM Storage**: All settings persist through power cycles

## Hardware Requirements

### Core Components
- **ESP32 Development Board** (ESP32-D0WD-V3 or similar)
- **128x32 RGB LED Matrix Panel** (HUB75 interface)
- **DS3231 RTC Module** (Real-Time Clock with battery backup)

### Input Controls
- **3 Push Buttons** for menu navigation:
  - Up Button (GPIO pin configurable)
  - Down Button (GPIO pin configurable) 
  - Select Button (GPIO pin configurable)

### Connections
```
ESP32 → LED Matrix (HUB75)
ESP32 → DS3231 (I2C: SDA/SCL)
ESP32 → Buttons (Digital inputs with pull-up)
```

## Dependencies

### PlatformIO Libraries
- `adafruit/Adafruit Protomatter@^1.6.4` - LED matrix driver
- `adafruit/RTClib@^2.1.4` - Real-time clock support
- `wire` - I2C communication

### Built-in Libraries
- `EEPROM` - Settings persistence
- `Arduino Framework` - ESP32 core functionality

## Installation

1. **Clone Repository**
   ```bash
   git clone <repository-url>
   cd "Matrix sign"
   ```

2. **Hardware Setup**
   - Connect LED matrix to ESP32 following HUB75 wiring
   - Connect DS3231 RTC module via I2C
   - Wire push buttons to configured GPIO pins

3. **Configure Pin Assignments**
   - Edit `src/main.cpp` to match your hardware connections
   - Update button pin definitions
   - Verify matrix pin assignments

4. **Build and Upload**
   ```bash
   pio run --target upload
   ```

## Usage

### Menu Navigation
- **Power On**: Displays current time immediately
- **Select Button**: Enter main menu
- **Up/Down**: Navigate menu options
- **Select**: Choose option or enter submenu
- **Long Press Select**: Exit menus and return to clock

### Menu Structure
```
Main Menu
├── Time Format (12H/24H)
├── Clock Color (16 colors + Rainbow)
├── Effects (8 background effects)
├── Text Size (1, 2, 3)
├── Brightness (10 levels)
└── Set Time (Hour/Minute adjustment)
```

### First-Time Setup
1. Set correct time using "Set Time" menu
2. Choose preferred time format (12H/24H)
3. Select clock color and text size
4. Adjust brightness for your environment
5. Pick a background effect (or leave off)

## Memory Usage
- **RAM**: ~8.1% (26,604 bytes)
- **Flash**: ~25.3% (331,957 bytes)
- **EEPROM**: Settings storage for persistence

## Technical Details

### Effect System
- Physics-based particle systems for realistic motion
- Brightness-aware rendering prevents effects from overpowering text
- Text masking ensures clock readability
- Frame-rate optimized for smooth animations

### Button Handling
- Debouncing for reliable input
- Repeat functionality for quick navigation
- State-aware processing prevents accidental triggers

### Time Management
- DS3231 RTC provides battery-backed timekeeping
- Automatic 12/24 hour conversion
- Leap year and daylight saving considerations

## Contributing

This project is open for improvements and feature additions. Key areas for enhancement:
- Additional background effects
- Weather integration
- WiFi time synchronization
- Custom color definitions
- Sound/alarm features

## License

[Specify your license here]

## Author

[Your name/contact information]

---

*Built with PlatformIO and Arduino Framework for ESP32*
