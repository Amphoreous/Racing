#pragma once

#include "core/Module.h"
#include "raylib.h"

class ModuleMainMenu : public Module
{
public:
    ModuleMainMenu(Application* app, bool start_enabled = true);
    ~ModuleMainMenu();

    bool Start() override;
    update_status Update() override;
    update_status PostUpdate() override;
    bool CleanUp() override;

private:
    enum MenuOption {
        START,
        OPTIONS,
        CREDITS,
        COUNT
    };

    enum MenuState {
        STATE_MAIN,
        STATE_OPTIONS,
        STATE_CREDITS
    };

    MenuOption currentSelection = START;
    MenuState currentState = STATE_MAIN;

    Texture2D backgroundTexture;
    Texture2D secondaryBackground;  // For options/credits screens
    Texture2D titleTexture;
    Texture2D buttonTextures[COUNT];         // Non-selected button textures
    Texture2D buttonSelectedTextures[COUNT]; // Selected button textures
    Texture2D selectingTexture;

    // Menu sound effects
    unsigned int selectSfx = 0;

    // Helper functions for sub-menus
    void DrawMainMenu();
    void DrawOptionsMenu();
    void DrawCreditsMenu();
};