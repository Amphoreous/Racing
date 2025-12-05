# Amphoreous Physics II Project: Pinball

![Pinball Logo](https://raw.githubusercontent.com/Amphoreous/Pinball/main/assets/ui/pinball_logo.png)

**Pinball** is a physics-based pinball simulation that recreates the excitement and challenge of the classic game. Experience realistic ball physics, responsive flipper controls, and engaging combo systems in this tribute to one of gaming's most beloved pinball experiences.

---

## Table of Contents

- [Team Members](#team-members)
- [Features](#features)
- [Installation](#installation)
- [Controls](#controls)
- [Gameplay](#gameplay)
- [Technical Implementation](#technical-implementation)
- [Development Stats](#development-stats)
- [Grading Requirements](#grading-requirements)
- [Contributing](#contributing)
- [Credits](#credits)
- [License](#license)
- [External Links & References](#external-links--references)

---

## Team Members

- **Zakaria Hamdaoui** ([TheUnrealZaka](https://github.com/TheUnrealZaka)) - Project Lead & Infrastructure
- **Sofia Giner Vargas** ([Katy-9](https://github.com/Katy-9)) - Art Lead & Visual Design
- **Joel Martínez Arjona** ([Jowey7](https://github.com/Jowey7)) - Physics Lead & Box2D Integration
- **Marc Pladellorens Pérez** ([MarcPladellorensPerez](https://github.com/MarcPladellorensPerez)) - Gameplay Lead & Systems Design
- **Montserrat Medina Chávez** ([montse4](https://github.com/montse4)) - Audio Lead & Polish

---

## Features

- **Realistic Pinball Physics:** Powered by Box2D engine for authentic ball movement and collisions
- **Responsive Controls:** Precision flipper control with left/right arrow keys
- **Comprehensive Scoring:** Current, previous, and highest score tracking with persistent storage
- **Ball Management System:** 3 balls per round with automatic cycling through multiple rounds
- **Complete Audio Experience:** Sound effects for hits, bonuses, and background music
- **Debug Visualization:** F1 toggle for physics shape visualization and mouse joint debugging
- **Real-Time Performance:** Stable 60 FPS gameplay with optimized physics simulation

---

## Installation

### Prerequisites

- **Operating Systems:** Windows 10/11
- **Hardware:** Minimum 1GB RAM, DirectX 9 compatible graphics card
- **Dependencies:** All required libraries (raylib, Box2D) are included in the release package

### Steps

1. **Download the Game:** Visit the [official releases page](https://github.com/Amphoreous/Pinball/releases) to get the latest version
2. **Extract Files:** Unzip the downloaded `GinerSofia_HamdaouiZakaria_MartinezJoel_MedinaMontserrat_PladellorensMarc_v1.0.zip` file
3. **Run the Game:**  
   - Double-click `Pinball.exe`

*Ensure all files remain in the same directory as the executable for proper asset loading.*

---

## Controls

### Gameplay Controls
- **Left Arrow Key:** Control left flipper
- **Right Arrow Key:** Control right flipper
- **Down Arrow Key:** Control kicker (launch ball)
- **P Key:** Pause/Unpause game
- **Space Key:** Start new game (from menu)

### Debug Controls
- **F1 Key:** Toggle debug mode (shows physics shapes and collision boundaries)
- **Mouse:** Drag physics objects when in debug mode (mouse joint functionality)

### Menu Navigation
- **Arrow Keys:** Navigate menus
- **Enter:** Select menu option
- **Escape:** Return to previous menu or pause game

---

## Gameplay

Experience authentic pinball action with modern physics simulation:

### Game Flow
- **Main Menu:** Start new game, view high scores, or access settings
- **Ball Launch:** Use DOWN arrow to launch ball from kicker
- **Flipper Control:** Use LEFT/RIGHT arrows for precise ball control
- **Scoring System:** Earn points through various targets and bumpers
- **Round Progression:** Complete rounds with 3 balls each, advance through multiple rounds

### Scoring System
- **Flipper Hits:** 10 points
- **Bumper Collisions:** 50 points
- **Target Hits:** 100 points

### Special Features
- **Extra Ball:** Earned through combo completion or high scores
- **High Score Persistence:** Your best scores are saved between sessions
- **Debug Mode:** F1 reveals physics boundaries for educational purposes

---

## Technical Implementation

### Core Technologies
- **Graphics & Audio:** [raylib](https://raylib.com/) - Simple and easy-to-use library
- **Physics Engine:** [Box2D](https://box2d.org/) - Industry-standard 2D physics simulation
- **Build System:** [premake5](https://premake.github.io/) - Cross-platform build configuration
- **Development Environment:** Visual Studio 2022, VSCode with C/C++ extensions

### Architecture
```
Pinball/
├── src/           # C++ source files (main.cpp, physics.cpp, game.cpp, audio.cpp)
├── include/       # Header files (game.h, physics.h, audio.h, resource_dir.h)
├── assets/        # Game assets (sprites, audio, ui)
├── build/         # Build configuration (premake5.lua)
└── bin/           # Compiled executables
```

### Performance Features
- **Fixed timestep physics** for consistent simulation
- **Efficient collision detection** using Box2D spatial partitioning  
- **Optimized rendering pipeline** with raylib
- **Memory management** with automatic cleanup and leak prevention

---

## Development Stats

- **Development Period:** 32 days (September 28 - October 30, 2025)
- **Total GitHub Issues:** 38 comprehensive issues completed
- **Team Members:** 5 specialized roles with clear responsibilities
- **Code Base:** C++ language with raylib and Box2D integration
- **Build Configurations:** Debug and Release modes with cross-platform support
- **Testing Platforms:** Windows (Visual Studio)

### Development Template
This project was built using the [raylib-box2d-quickstart](https://github.com/TheUnrealZaka/raylib-box2d-quickstart) template by @TheUnrealZaka, providing a solid foundation for raylib and Box2D integration.

---

## Grading Requirements

### ✅ Mandatory Requirements Met

**Core Functionality:**
- ✅ **No crashes** during extended gameplay (tested 30+ minutes)
- ✅ **Flipper controls** respond immediately to left/right arrow keys
- ✅ **Kicker control** responds to down arrow key for ball launching
- ✅ **Clear game begin/end** states with proper transitions

**Scoring & Gameplay:**
- ✅ **Current score** updates live during gameplay
- ✅ **Previous score** displayed after each round ends
- ✅ **Highest score** persists between game sessions
- ✅ **Specific ball count** per round (3 balls) with automatic cycling
- ✅ **At least one combo system** (POKEMON letter collection for bonuses)

**Technical Features:**
- ✅ **F1 debug mode** shows all physics shapes and collision boundaries
- ✅ **Mouse joint functionality** allows dragging objects in debug mode
- ✅ **Real-time performance** maintains stable 60 FPS without bullet-time
- ✅ **Audio effects** for hits, bonuses, and combo completions
- ✅ **Memory leak free** verified with testing tools

**Documentation:**
- ✅ **All team member names** listed in README and release
- ✅ **GitHub repository link** included in all documentation
- ✅ **Complete control instructions** documented
- ✅ **Differences from original** clearly explained

---

## Contributing

We welcome contributions to enhance the Pokemon Pinball Clone. Follow these steps to get involved:

1. **Fork the Repository**
2. **Create a Feature Branch:**
   ```bash
   git checkout -b feature/YourFeatureName
   ```
3. **Commit Your Changes:**
   ```bash
   git commit -m "Add YourFeatureName"
   ```
4. **Push to Your Branch:**
   ```bash
   git push origin feature/YourFeatureName
   ```
5. **Submit a Pull Request**  
   Please include documentation and tests for your changes.

### Development Guidelines
- Follow C++ coding standards and consistent formatting
- Test on multiple platforms before submitting
- Ensure no memory leaks with Valgrind or similar tools
- Update documentation for any new features
- Maintain 60 FPS performance standards

---

## Credits

### Development Team
- **Zakaria Hamdaoui** - Project architecture, build system, CI/CD, debug features
- **Sofia Giner Vargas** - Visual design, sprite creation, UI elements, asset management
- **Joel Martínez Arjona** - Box2D integration, physics tuning, performance optimization
- **Marc Pladellorens Pérez** - Game logic, scoring system, combo mechanics, state management  
- **Montserrat Medina Chávez** - Audio system, sound effects, music integration, final polish

### Special Thanks
- **Template Foundation:** @TheUnrealZaka's raylib-box2d-quickstart
- **Libraries:** raylib community, Box2D developers
- **Educational Support:** Game Design and Development course instructors and peers
---

## License

This project's **source code** is licensed under the MIT License. All game assets (sprites, audio, visual elements) are used for educational purposes only and remain the property of their respective owners.

> **Educational Note:** This project is developed as part of an academic assignment for the Game Design and Development course. All Pokemon-related assets and references are used solely for educational purposes and are not licensed under the MIT License.

---

## External Links & References

- **GitHub Repository:** [https://github.com/Amphoreous/Pinball](https://github.com/Amphoreous/Pinball)
- **Development Template:** [raylib-box2d-quickstart](https://github.com/TheUnrealZaka/raylib-box2d-quickstart)
- **raylib Documentation:** [https://raylib.com/](https://raylib.com/)
- **Box2D Documentation:** [https://box2d.org/documentation/](https://box2d.org/documentation/)
---

_For technical support or questions about the game, please open an issue on GitHub or contact the development team._