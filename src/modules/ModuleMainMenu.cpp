#include "modules/ModuleMainMenu.h"
#include "core/Application.h"
#include "modules/ModuleResources.h"
#include "modules/ModuleGame.h"
#include "modules/ModulePhysics.h"
#include "modules/ModuleAudio.h"
#include "entities/Player.h"
#include "entities/NPCManager.h"
#include "core/Map.h"
#include "entities/CheckpointManager.h"
#include "raylib.h"

ModuleMainMenu::ModuleMainMenu(Application* app, bool start_enabled) : Module(app, start_enabled)
{
    LOG("Main menu constructor");
    currentState = STATE_MAIN;
}

ModuleMainMenu::~ModuleMainMenu()
{
}

bool ModuleMainMenu::Start()
{
    LOG("Main menu started");
    currentState = STATE_MAIN;
    currentSelection = START;
    backgroundTexture = App->resources->LoadTexture("assets/ui/backgrounds/main_menu_background.jpg");
    secondaryBackground = App->resources->LoadTexture("assets/ui/backgrounds/second_background.png");
    titleTexture = App->resources->LoadTexture("assets/ui/hud/main_menu_title.png");
    
    // Load non-selected button textures (always visible)
    buttonTextures[START] = App->resources->LoadTexture("assets/ui/hud/main_menu_start.png");
    buttonTextures[OPTIONS] = App->resources->LoadTexture("assets/ui/hud/main_menu_options.png");
    buttonTextures[CREDITS] = App->resources->LoadTexture("assets/ui/hud/main_menu_credits.png");
    
    // Load selected button textures (shown when option is highlighted)
    buttonSelectedTextures[START] = App->resources->LoadTexture("assets/ui/hud/main_menu_start_selected.png");
    buttonSelectedTextures[OPTIONS] = App->resources->LoadTexture("assets/ui/hud/main_menu_options_selected.png");
    buttonSelectedTextures[CREDITS] = App->resources->LoadTexture("assets/ui/hud/main_menu_credits_selected.png");
    
    selectingTexture = App->resources->LoadTexture("assets/ui/hud/main_menu_selecting.png");

    // Load menu sound effects
    selectSfx = App->audio->LoadFx("assets/audio/fx/checkpoint.wav");

    // Play main menu music
    App->audio->PlayMusic("assets/audio/music/main_menu_music.mp3");
    
    return true;
}

update_status ModuleMainMenu::Update()
{
    switch (currentState)
    {
    case STATE_MAIN:
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
            App->audio->PlayFx(selectSfx);
            
            if (currentSelection == START)
            {
                LOG("Starting game from menu");
                App->state = GAME_PLAYING;
                
                // CRITICAL: Enable in correct order!
                // 1. Map FIRST - loads positions and collision data
                App->map->Enable();
                
                // 2. Physics SECOND - creates world for bodies
                App->physics->Enable();
                
                // 3. Game scene
                App->scene_intro->Enable();
                
                // 4. Player and NPCs - need map positions!
                App->player->Enable();
                App->npcManager->Enable();
                
                // 5. Checkpoint manager - needs player reference
                App->checkpointManager->Enable();
                
                this->Disable();
            }
            else if (currentSelection == OPTIONS)
            {
                LOG("Opening Options menu");
                currentState = STATE_OPTIONS;
            }
            else if (currentSelection == CREDITS)
            {
                LOG("Opening Credits menu");
                currentState = STATE_CREDITS;
            }
        }
        break;

    case STATE_OPTIONS:
    case STATE_CREDITS:
        // Press ESC or BACKSPACE to return to main menu
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE))
        {
            LOG("Returning to main menu");
            currentState = STATE_MAIN;
        }
        break;
    }

    return UPDATE_CONTINUE;
}

update_status ModuleMainMenu::PostUpdate()
{
    switch (currentState)
    {
    case STATE_MAIN:
        DrawMainMenu();
        break;
    case STATE_OPTIONS:
        DrawOptionsMenu();
        break;
    case STATE_CREDITS:
        DrawCreditsMenu();
        break;
    }
    return UPDATE_CONTINUE;
}

void ModuleMainMenu::DrawMainMenu()
{
    Rectangle source = {0, 0, 1890, 1417};
    Rectangle dest = {0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
    Vector2 origin = {0, 0};
    DrawTexturePro(backgroundTexture, source, dest, origin, 0, WHITE);
    DrawTexturePro(titleTexture, source, dest, origin, 0, WHITE);
    
    // Draw all three menu options - use selected texture for current selection, normal for others
    for (int i = 0; i < COUNT; i++)
    {
        if (i == currentSelection)
        {
            DrawTexturePro(buttonSelectedTextures[i], source, dest, origin, 0, WHITE);
        }
        else
        {
            DrawTexturePro(buttonTextures[i], source, dest, origin, 0, WHITE);
        }
    }
    
    // Draw the selector indicator
    Rectangle selectingDest = dest;
    if (currentSelection == OPTIONS) {
        selectingDest.y += 103;
    }
    else if (currentSelection == CREDITS)
    {
        selectingDest.y += 206;
    }
    DrawTexturePro(selectingTexture, source, selectingDest, origin, 0, WHITE);

    // Draw copyright notice at the bottom
    const char* copyright = "(c) Copyright. Amphoreous 2025. All rights reserved.";
    int copyrightFontSize = 20;
    float footerY = SCREEN_HEIGHT - 40;
    DrawText(copyright, 20, (int)footerY, copyrightFontSize, WHITE);
}

void ModuleMainMenu::DrawOptionsMenu()
{
    // Draw secondary background
    Rectangle source = {0, 0, (float)secondaryBackground.width, (float)secondaryBackground.height};
    Rectangle dest = {0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
    Vector2 origin = {0, 0};
    DrawTexturePro(secondaryBackground, source, dest, origin, 0, WHITE);

    // Semi-transparent overlay
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));

    // Title
    const char* title = "CONTROLS";
    int titleFontSize = 60;
    int titleWidth = MeasureText(title, titleFontSize);
    DrawText(title, (SCREEN_WIDTH - titleWidth) / 2, 60, titleFontSize, GOLD);

    // Controls list
    int startY = 160;
    int lineHeight = 40;
    int fontSize = 28;
    int col1X = SCREEN_WIDTH / 2 - 300;
    int col2X = SCREEN_WIDTH / 2 + 50;

    const char* controls[][2] = {
        {"Accelerate", "W / Up Arrow"},
        {"Brake / Reverse", "S / Down Arrow"},
        {"Steer Left", "A / Left Arrow"},
        {"Steer Right", "D / Right Arrow"},
        {"Change Camera", "C"},
        {"Push Ability", "Space"},
        {"Toggle Debug", "F1"},
        {"Pause", "P"},
        {"Back / Menu", "Escape"}
    };

    int numControls = sizeof(controls) / sizeof(controls[0]);
    for (int i = 0; i < numControls; i++)
    {
        DrawText(controls[i][0], col1X, startY + i * lineHeight, fontSize, WHITE);
        DrawText(controls[i][1], col2X, startY + i * lineHeight, fontSize, SKYBLUE);
    }

    // Debug info section
    int debugY = startY + numControls * lineHeight + 40;
    DrawText("DEBUG MODE (F1):", col1X, debugY, fontSize, YELLOW);
    debugY += lineHeight;
    DrawText("- Shows physics collision shapes", col1X + 20, debugY, fontSize - 4, LIGHTGRAY);
    debugY += lineHeight - 8;
    DrawText("- Drag objects with mouse", col1X + 20, debugY, fontSize - 4, LIGHTGRAY);
    debugY += lineHeight - 8;
    DrawText("- FPS and physics stats overlay", col1X + 20, debugY, fontSize - 4, LIGHTGRAY);

    // Back instruction
    const char* backText = "Press ESC or BACKSPACE to return";
    int backWidth = MeasureText(backText, 24);
    DrawText(backText, (SCREEN_WIDTH - backWidth) / 2, SCREEN_HEIGHT - 60, 24, GRAY);
}

void ModuleMainMenu::DrawCreditsMenu()
{
    // Draw secondary background
    Rectangle source = {0, 0, (float)secondaryBackground.width, (float)secondaryBackground.height};
    Rectangle dest = {0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
    Vector2 origin = {0, 0};
    DrawTexturePro(secondaryBackground, source, dest, origin, 0, WHITE);

    // Semi-transparent overlay
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.6f));

    // Game title
    const char* gameTitle = "LUMA GRAND PRIX";
    int gameTitleFontSize = 50;
    int gameTitleWidth = MeasureText(gameTitle, gameTitleFontSize);
    DrawText(gameTitle, (SCREEN_WIDTH - gameTitleWidth) / 2, 50, gameTitleFontSize, GOLD);

    // Subtitle
    const char* subtitle = "Game Development Course Final Project";
    int subtitleWidth = MeasureText(subtitle, 24);
    DrawText(subtitle, (SCREEN_WIDTH - subtitleWidth) / 2, 110, 24, LIGHTGRAY);

    // Team section
    int teamY = 170;
    const char* teamTitle = "TEAM MEMBERS";
    int teamTitleWidth = MeasureText(teamTitle, 36);
    DrawText(teamTitle, (SCREEN_WIDTH - teamTitleWidth) / 2, teamY, 36, WHITE);

    int memberY = teamY + 50;
    int memberFontSize = 26;
    int roleSize = 20;
    int centerX = SCREEN_WIDTH / 2;

    // Team members with roles
    struct TeamMember {
        const char* name;
        const char* role;
        const char* github;
    };

    TeamMember members[] = {
        {"Zakaria Hamdaoui", "Project Lead & Infrastructure", "@TheUnrealZaka"},
        {"Sofia Giner Vargas", "Art & Visuals", "@Katy-9"},
        {"Joel Martinez Arjona", "Physics & Box2D Integration", "@Jowey7"}
    };

    for (int i = 0; i < 3; i++)
    {
        int nameWidth = MeasureText(members[i].name, memberFontSize);
        DrawText(members[i].name, centerX - nameWidth / 2, memberY, memberFontSize, SKYBLUE);
        
        int roleWidth = MeasureText(members[i].role, roleSize);
        DrawText(members[i].role, centerX - roleWidth / 2, memberY + 30, roleSize, LIGHTGRAY);
        
        int githubWidth = MeasureText(members[i].github, roleSize - 2);
        DrawText(members[i].github, centerX - githubWidth / 2, memberY + 52, roleSize - 2, DARKGRAY);
        
        memberY += 90;
    }

    // Technologies section
    int techY = memberY + 20;
    const char* techTitle = "BUILT WITH";
    int techTitleWidth = MeasureText(techTitle, 30);
    DrawText(techTitle, (SCREEN_WIDTH - techTitleWidth) / 2, techY, 30, WHITE);

    techY += 40;
    const char* techs[] = {"raylib - Graphics & Audio", "Box2D - 2D Physics Engine", "Tiled - Map Editor"};
    for (int i = 0; i < 3; i++)
    {
        int techWidth = MeasureText(techs[i], 22);
        DrawText(techs[i], centerX - techWidth / 2, techY + i * 30, 22, LIGHTGRAY);
    }

    // Links section
    int linksY = techY + 110;
    const char* repoText = "Repository: github.com/Amphoreous/Racing";
    int repoWidth = MeasureText(repoText, 20);
    DrawText(repoText, centerX - repoWidth / 2, linksY, 20, GRAY);

    const char* itchText = "itch.io: amphoreous.itch.io/luma-grand-prix";
    int itchWidth = MeasureText(itchText, 20);
    DrawText(itchText, centerX - itchWidth / 2, linksY + 28, 20, GRAY);

    // License
    const char* license = "MIT License - 2025";
    int licenseWidth = MeasureText(license, 18);
    DrawText(license, centerX - licenseWidth / 2, linksY + 65, 18, DARKGRAY);

    // Back instruction
    const char* backText = "Press ESC or BACKSPACE to return";
    int backWidth = MeasureText(backText, 24);
    DrawText(backText, (SCREEN_WIDTH - backWidth) / 2, SCREEN_HEIGHT - 50, 24, GRAY);
}

bool ModuleMainMenu::CleanUp()
{
    currentState = STATE_MAIN;
    App->resources->UnloadTexture("assets/ui/backgrounds/main_menu_background.jpg");
    App->resources->UnloadTexture("assets/ui/backgrounds/second_background.png");
    App->resources->UnloadTexture("assets/ui/hud/main_menu_title.png");
    App->resources->UnloadTexture("assets/ui/hud/main_menu_start_selected.png");
    App->resources->UnloadTexture("assets/ui/hud/main_menu_options_selected.png");
    App->resources->UnloadTexture("assets/ui/hud/main_menu_credits_selected.png");
    App->resources->UnloadTexture("assets/ui/hud/main_menu_selecting.png");
    return true;
}