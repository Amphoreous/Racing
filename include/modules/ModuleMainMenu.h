#pragma once

#include "core/Module.h"

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

    MenuOption currentSelection = START;

    Texture2D backgroundTexture;
    Texture2D titleTexture;
    Texture2D buttonTextures[COUNT];
    Texture2D selectingTexture;
};