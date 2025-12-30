#include "modules/ModuleMainMenu.h"
#include "core/Application.h"
#include "modules/ModuleResources.h"
#include "modules/ModuleGame.h"
#include "modules/ModulePhysics.h"
#include "entities/Player.h"
#include "entities/NPCManager.h"
#include "core/Map.h"
#include "entities/CheckpointManager.h"
#include "raylib.h"

ModuleMainMenu::ModuleMainMenu(Application* app, bool start_enabled) : Module(app, start_enabled)
{
    LOG("Main menu constructor");
}

ModuleMainMenu::~ModuleMainMenu()
{
}

bool ModuleMainMenu::Start()
{
    LOG("Main menu started");
    backgroundTexture = App->resources->LoadTexture("assets/ui/backgrounds/main_menu_background.jpg");
    titleTexture = App->resources->LoadTexture("assets/ui/hud/main_menu_title.png");
    buttonTextures[START] = App->resources->LoadTexture("assets/ui/hud/main_menu_start_selected.png");
    buttonTextures[OPTIONS] = App->resources->LoadTexture("assets/ui/hud/main_menu_options_selected.png");
    buttonTextures[CREDITS] = App->resources->LoadTexture("assets/ui/hud/main_menu_credits_selected.png");
    selectingTexture = App->resources->LoadTexture("assets/ui/hud/main_menu_selecting.png");
    return true;
}

update_status ModuleMainMenu::Update()
{
    if (IsKeyPressed(KEY_DOWN))
    {
        currentSelection = (MenuOption)((currentSelection + 1) % COUNT);
    }
    if (IsKeyPressed(KEY_UP))
    {
        currentSelection = (MenuOption)((currentSelection - 1 + COUNT) % COUNT);
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
    {
        if (currentSelection == START)
        {
            LOG("Starting game from menu");
            // Start the game
            App->state = GAME_PLAYING;
            App->scene_intro->Enable();
            App->physics->Enable();
            App->player->Enable();
            App->npcManager->Enable();
            App->map->Enable();
            App->checkpointManager->Enable();
            // Disable menu
            this->Disable();
        }
        // For others, do nothing
    }
    return UPDATE_CONTINUE;
}

update_status ModuleMainMenu::PostUpdate()
{
    // Draw scaled to fit screen
    Rectangle source = {0, 0, 1890, 1417};
    Rectangle dest = {0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
    Vector2 origin = {0, 0};
    DrawTexturePro(backgroundTexture, source, dest, origin, 0, WHITE);
    DrawTexturePro(titleTexture, source, dest, origin, 0, WHITE);
    DrawTexturePro(buttonTextures[currentSelection], source, dest, origin, 0, WHITE);
    Rectangle selectingDest = dest;
    if (currentSelection == OPTIONS) {
        selectingDest.y += 103;
    }
    else if (currentSelection == CREDITS)
    {
        selectingDest.y += 206;
    }
    DrawTexturePro(selectingTexture, source, selectingDest, origin, 0, WHITE);
    return UPDATE_CONTINUE;
}

bool ModuleMainMenu::CleanUp()
{
    App->resources->UnloadTexture("assets/ui/backgrounds/main_menu_background.jpg");
    App->resources->UnloadTexture("assets/ui/hud/main_menu_title.png");
    App->resources->UnloadTexture("assets/ui/hud/main_menu_start_selected.png");
    App->resources->UnloadTexture("assets/ui/hud/main_menu_options_selected.png");
    App->resources->UnloadTexture("assets/ui/hud/main_menu_credits_selected.png");
    App->resources->UnloadTexture("assets/ui/hud/main_menu_selecting.png");
    return true;
}