# Amphoreous — Racing

A lightweight 2D arcade racing project built with Raylib and Box2D. Amphoreous/Racing focuses on simple, responsive driving mechanics, deterministic physics, modular architecture, and easy-to-author tracks using Tiled.

Quick highlights
- Physics-driven car handling (Box2D via project physics wrappers)
- Tiled map support (chain/polyline boundaries)
- Player and AI opponents (waypoint system)
- Debug tools (F1: physics visualization, mouse joint dragging)
- Simple UI (lap timer, speedometer, menus)
- Audio support (background music and SFX)

---

## Table of contents
- [Team](#team)
- [Goals & Features](#goals--features)
- [Getting started](#getting-started)
- [Controls](#controls)
- [Project structure](#project-structure)
- [Technical notes](#technical-notes)
- [Development stats](#development-stats)
- [Contributing](#contributing)
- [Credits & License](#credits--license)
- [Contact](#contact)

---

## Team
This project is currently maintained by a core team of three contributors:

- Zakaria Hamdaoui — Project Lead & Infrastructure (@TheUnrealZaka)
- Sofia Giner Vargas — Art & Visuals (@Katy-9)
- Joel Martínez Arjona — Physics & Box2D Integration (@Jowey7)

---

## Goals & Features
This repository implements a compact 2D racing experience with emphasis on:
- Accurate and stable physics integration using Box2D (through project-specific wrappers).
- Clean separation of concerns: Resource management, Physics wrappers, Game logic, Rendering, and UI.
- Track authoring via Tiled with chain/loop collision conversion (pixels → meters).
- Tunable car mechanics: acceleration, steering, drifting (lateral impulse), max speed clamps.
- AI that follows waypoints placed in maps (simple opponent logic with rubber-banding).
- Debugging tools for rapid iteration (physics draw, mouse joint dragging).
- Simple HUD with lap timer and speedometer; main menu and victory screens.
- Audio system using a Resource Manager to load/unload SFX and music safely.

---

## Getting started

Prerequisites
- C++ toolchain (Visual Studio 2022 on Windows, or GCC/Clang on Linux)
- premake5 (optional if using the provided build configuration)
- raylib (linked or installed)
- Box2D (linked or installed)

Basic build steps (typical)
1. git clone https://github.com/Amphoreous/Racing.git
2. Install or make available raylib and Box2D for your platform.
3. Use premake5 (if included) to generate the appropriate solution/Makefile:
   - Windows (Visual Studio): premake5 vs2022
   - Linux/macOS: premake5 gmake2 && make -C build
4. Open the generated solution or run make to compile.
5. Run the produced executable from the `bin/` directory.

Notes
- The project aims to keep dependencies minimal. If runtime issues occur, ensure raylib and Box2D are compatible with your platform and build mode (Debug/Release).
- Asset paths are relative to the executable; keep the assets folder next to the binary.

---

## Controls

Gameplay
- Accelerate: W / Up arrow
- Brake / Reverse: S / Down arrow
- Steer left/right: A / Left arrow, D / Right arrow
- Pause: P
- Restart / Confirm: Space / Enter
- Escape: Back / Open menu

Debug
- Toggle debug draw: F1
- Drag physics bodies with mouse while in debug mode

Menu navigation
- Use arrow keys or WASD to navigate UI and Enter to select.

---

## Project structure

A high-level view of the repository:

```
Amphoreous/Racing/
├── src/           # C++ source (game loop, modules, wrappers)
├── include/       # Public headers
├── assets/        # Sprites, Tiled maps, audio
├── scripts/       # Helper build / automation scripts
├── build/         # Generated build files (premake, makefiles)
└── bin/           # Compiled binaries / runtime output
```

Module responsibilities (convention)
- ModuleResources / Resource Manager — centralized loading/unloading of textures, audio, maps.
- ModulePhysics — Box2D world and wrappers (PhysBody, helper functions).
- Entities — PlayerCar, AICar, Track objects — each entity owns update/draw hooks.
- UI — HUD, menus, overlays.
- MapLoader — Tiled parsing and chain generation for static level boundaries.

---

## Technical notes & best practices

- Physics: Fixed timestep simulation to keep deterministic behavior across frame rates.
- Units: Store physics in meters; convert pixels ↔ meters at object creation (`PIXEL_TO_METERS`).
- Rendering: Use raylib DrawTexturePro and convert Box2D radians → degrees for sprite rotation.
- Audio: Load via Resource Manager to prevent leaks. Test .ogg and .wav decoding on your platform.
- Debugging: Provide clear logging around module initialization and asset loading failures to troubleshoot runtime errors.
- 
---

## Development stats
- Active issues: 25 open issues related to architecture, physics, gameplay, AI, UI, audio, and release tasks — check the repository Issues tab for details.
- Key development areas: Architecture (wrappers & resource manager), Gameplay & Physics, Map loading, AI, UI, Audio, and Release packaging.

---

## Contributing
We welcome contributions. Typical workflow:
1. Fork the repository and create a topic branch: git checkout -b feature/your-feature
2. Keep commits small and well-scoped. Include tests where practical.
3. Ensure no memory leaks and adhere to the project's coding conventions.
4. Open a pull request describing the change and link related issues.

Before contributing:
- Run the game locally and ensure your change does not break the build in Debug and Release mode.
- Follow code formatting and document public APIs.

---

## Credits & License
Core libraries:
- raylib — graphics and audio
- Box2D — 2D physics engine

This project is provided under the MIT License. See the LICENSE file for details.

---

## Contact
For issues, feature requests, or support, open an issue in this repository: https://github.com/Amphoreous/Racing/issues