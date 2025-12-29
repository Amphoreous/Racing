#include "core/Globals.h"
#include "core/Application.h"
#include "modules/ModuleWindow.h"
#include "modules/ModuleRender.h"
#include <math.h>

ModuleRender::ModuleRender(Application* app, bool start_enabled) : Module(app, start_enabled)
{
    background = RAYWHITE;
    camera = { 0, 0, 0, 0 };  // Initialize camera
}

ModuleRender::~ModuleRender()
{
}

bool ModuleRender::Init()
{
    LOG("Setting up renderer");
    return true;
}

update_status ModuleRender::PreUpdate()
{
    return UPDATE_CONTINUE;
}

update_status ModuleRender::Update()
{
    ClearBackground(background);
    BeginDrawing();  // Start drawing batch for better performance
    return UPDATE_CONTINUE;
}

update_status ModuleRender::PostUpdate()
{
    EndDrawing();  // Render everything at once
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

    position.x = (float)(x - pivot_x) * scale + camera.x;
    position.y = (float)(y - pivot_y) * scale + camera.y;

    if (section != NULL)
    {
        rect.width *= scale;
        rect.height *= scale;
    }

    // LOG("Drawing texture at (%.1f, %.1f), size: %.1fx%.1f", position.x, position.y, rect.width, rect.height);

    DrawTextureRec(texture, rect, position, WHITE);

    return ret;
}

bool ModuleRender::DrawText(const char* text, int x, int y, Font font, int spacing, Color tint) const
{
    bool ret = true;

    Vector2 position = { (float)x, (float)y };

    DrawTextEx(font, text, position, (float)font.baseSize, (float)spacing, tint);

    return ret;
}