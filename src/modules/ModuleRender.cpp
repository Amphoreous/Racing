#include "core/Globals.h"
#include "core/Application.h"
#include "core/Map.h"
#include "modules/ModuleWindow.h"
#include "modules/ModuleRender.h"
#include "modules/ModuleGame.h"
#include "modules/ModulePhysics.h"
#include "entities/Player.h"
#include "entities/Car.h"
#include <math.h>

ModuleRender::ModuleRender(Application* app, bool start_enabled) : Module(app, start_enabled)
{
    background = RAYWHITE;
    camera.target = { 0, 0 };
    camera.offset = { (float)SCREEN_WIDTH * 0.5f, (float)SCREEN_HEIGHT * 0.5f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
}

ModuleRender::~ModuleRender()
{
}

bool ModuleRender::Init()
{
    LOG("Setting up renderer");
    return true;
}

bool ModuleRender::Start()
{
    // Initialize camera centered on player at start
    UpdateCamera();
    return true;
}

update_status ModuleRender::PreUpdate()
{
    return UPDATE_CONTINUE;
}

update_status ModuleRender::Update()
{
    // Update camera position to follow player BEFORE drawing anything
    UpdateCamera();

    ClearBackground(background);
    BeginMode2D(camera);

    // Render the background within camera space so it follows the camera
    if (App && App->scene_intro)
    {
        App->scene_intro->RenderTiledBackground();
    }

    // Render the map within camera space so it follows the camera
    if (App && App->map)
    {
        App->map->RenderMap();
    }

    // Render physics debug visualization within camera space
    if (App && App->physics)
    {
        App->physics->DebugDraw();
    }

    return UPDATE_CONTINUE;
}

update_status ModuleRender::PostUpdate()
{
    EndMode2D();

    // Render physics debug overlay in screen space (after camera transformations)
    if (App && App->physics)
    {
        App->physics->RenderDebug();
    }

    EndDrawing();
    return UPDATE_CONTINUE;
}

bool ModuleRender::CleanUp()
{
    return true;
}

void ModuleRender::SetBackgroundColor(Color color)
{
    background = color;
}

void ModuleRender::UpdateCamera()
{
    if (!App || !App->player || !App->player->GetCar())
        return;

    // Get player car position and rotation
    float playerX, playerY;
    App->player->GetCar()->GetPosition(playerX, playerY);
    float playerRotation = App->player->GetCar()->GetRotation();

    // Set camera target to player position
    camera.target = { playerX, playerY };

    // Set camera rotation to match player car rotation (negated for correct direction)
    camera.rotation = -playerRotation;
}

// Draw to screen
bool ModuleRender::Draw(Texture2D texture, int x, int y, const Rectangle* section, double angle, int pivot_x, int pivot_y) const
{
    bool ret = true;

    float scale = 1.0f;
    Vector2 position = { (float)x, (float)y };
    Rectangle rect = { 0.f, 0.f, (float)texture.width, (float)texture.height };

    if (section != NULL)
    {
        rect = *section;
    }

    // With BeginMode2D, position is in world coordinates
    position.x = (float)(x - pivot_x) * scale;
    position.y = (float)(y - pivot_y) * scale;

    if (section != NULL)
    {
        rect.width *= scale;
        rect.height *= scale;
    }

    DrawTextureRec(texture, rect, position, WHITE);

    return ret;
}

bool ModuleRender::DrawText(const char* text, int x, int y, Font font, int spacing, Color tint) const
{
    bool ret = true;

    Vector2 position = { (float)x - camera.target.x, (float)y - camera.target.y };

    DrawTextEx(font, text, position, (float)font.baseSize, (float)spacing, tint);

    return ret;
}