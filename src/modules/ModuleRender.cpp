#include "core/Globals.h"
#include "core/Application.h"
#include "modules/ModuleWindow.h"
#include "modules/ModuleRender.h"
#include "entities/Player.h"
#include "entities/Car.h"
#include <math.h>

ModuleRender::ModuleRender(Application* app, bool start_enabled) : Module(app, start_enabled)
{
    background = RAYWHITE;
    camera = { 0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT };
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
    BeginDrawing();
    return UPDATE_CONTINUE;
}

update_status ModuleRender::PostUpdate()
{
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

    // Get player car position (this is updated by physics)
    float playerX, playerY;
    App->player->GetCar()->GetPosition(playerX, playerY);

    // Center camera on player - camera.x and camera.y represent the top-left corner
    camera.x = playerX - (SCREEN_WIDTH * 0.5f);
    camera.y = playerY - (SCREEN_HEIGHT * 0.5f);
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

    position.x = (float)(x - pivot_x) * scale - camera.x;
    position.y = (float)(y - pivot_y) * scale - camera.y;

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

    Vector2 position = { (float)x - camera.x, (float)y - camera.y };

    DrawTextEx(font, text, position, (float)font.baseSize, (float)spacing, tint);

    return ret;
}