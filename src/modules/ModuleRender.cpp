#include "core/Globals.h"
#include "core/Application.h"
#include "core/Map.h"
#include "modules/ModuleWindow.h"
#include "modules/ModuleRender.h"
#include "modules/ModuleGame.h"
#include "modules/ModulePhysics.h"
#include "entities/Player.h"
#include "entities/Car.h"
#include "entities/CheckpointManager.h"
#include <math.h>

ModuleRender::ModuleRender(Application* app, bool start_enabled) : Module(app, start_enabled)
{
    background = RAYWHITE;
    camera.target = { 0, 0 };
    camera.offset = { (float)SCREEN_WIDTH * 0.5f, (float)SCREEN_HEIGHT * 0.5f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
    cameraMode = CAMERA_FOLLOW_CAR;  // Start with car-following mode
    screenShakeAmount = 0.0f;
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
    // Handle camera mode switching input
    HandleCameraInput();
    
    // Update camera position to follow player BEFORE drawing anything
    UpdateCamera();

    // Apply screen shake to camera offset
    Vector2 shakeOffset = {0, 0};
    if (screenShakeAmount > 0)
    {
        shakeOffset.x = GetRandomValue(-screenShakeAmount, screenShakeAmount);
        shakeOffset.y = GetRandomValue(-screenShakeAmount, screenShakeAmount);
    }
    camera.offset.x = (float)SCREEN_WIDTH * 0.5f + shakeOffset.x;
    camera.offset.y = (float)SCREEN_HEIGHT * 0.5f + shakeOffset.y;

    ClearBackground(background);

    // In full map view, render background in screen space (static)
    if (cameraMode == CAMERA_FULL_MAP && App && App->scene_intro)
    {
        // Render background centered on screen for full map view
        App->scene_intro->RenderTiledBackground(true);  // screenSpace = true
    }

    BeginMode2D(camera);

    // In follow car modes, render background within camera space (follows camera)
    if (cameraMode != CAMERA_FULL_MAP && App && App->scene_intro)
    {
        App->scene_intro->RenderTiledBackground(false);  // screenSpace = false
    }

    // Render the map within camera space so it follows the camera
    if (App && App->map)
    {
        App->map->RenderMap();
    }

    // Render physics debug visualization within camera space
    if (App && App->physics && App->physics->IsDebugMode())
    {
        App->physics->DebugDraw();
    }

    return UPDATE_CONTINUE;
}

update_status ModuleRender::PostUpdate()
{
    EndMode2D();

    // Render physics debug overlay in screen space (after camera transformations)
    if (App && App->physics && App->physics->IsDebugMode())
    {
        App->physics->RenderDebug();
    }

    // Render win screen in screen space (covers everything)
    if (App && App->checkpointManager && App->checkpointManager->IsRaceFinished())
    {
        App->checkpointManager->DrawWinScreen();
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

void ModuleRender::HandleCameraInput()
{
    // Cycle through camera modes with C key
    if (IsKeyPressed(KEY_C))
    {
        if (cameraMode == CAMERA_FOLLOW_CAR)
        {
            cameraMode = CAMERA_FOLLOW_CAR_NO_ROT;
            LOG("Camera mode: Follow Car (No Rotation)");
        }
        else if (cameraMode == CAMERA_FOLLOW_CAR_NO_ROT)
        {
            cameraMode = CAMERA_FULL_MAP;
            LOG("Camera mode: Full Map View");
        }
        else
        {
            cameraMode = CAMERA_FOLLOW_CAR;
            LOG("Camera mode: Follow Car (With Rotation)");
        }
    }
}

void ModuleRender::AddScreenShake(float amount)
{
    screenShakeAmount += amount;
}

void ModuleRender::UpdateCamera()
{
    if (!App)
        return;

    // Update screen shake
    if (screenShakeAmount > 0)
    {
        screenShakeAmount -= 50.0f * GetFrameTime(); // Reduce over time
        if (screenShakeAmount < 0) screenShakeAmount = 0;
    }

    if (cameraMode == CAMERA_FOLLOW_CAR)
    {
        // Follow car mode - existing behavior
        if (!App->player || !App->player->GetCar())
            return;

        // Get player car position and rotation
        float playerX, playerY;
        App->player->GetCar()->GetPosition(playerX, playerY);
        float playerRotation = App->player->GetCar()->GetRotation();

        // Smooth camera target
        float smoothSpeed = 0.125f;
        camera.target.x = camera.target.x + (playerX - camera.target.x) * smoothSpeed;
        camera.target.y = camera.target.y + (playerY - camera.target.y) * smoothSpeed;

        // Set camera rotation to match player car rotation (negated for correct direction)
        camera.rotation = -playerRotation;
        
        // Reset zoom to normal
        camera.zoom = 1.0f;
    }
    else if (cameraMode == CAMERA_FOLLOW_CAR_NO_ROT)
    {
        // Follow car mode without rotation
        if (!App->player || !App->player->GetCar())
            return;

        // Get player car position
        float playerX, playerY;
        App->player->GetCar()->GetPosition(playerX, playerY);

        // Set camera target to player position
        camera.target = { playerX, playerY };

        // No rotation in this mode
        camera.rotation = 0.0f;
        
        // Reset zoom to normal
        camera.zoom = 1.0f;
    }
    else if (cameraMode == CAMERA_FULL_MAP)
    {
        // Full map mode - show entire map
        // Get map dimensions dynamically from loaded map
        float mapWidth = 5500.0f;  // Default fallback
        float mapHeight = 3360.0f; // Default fallback
        
        if (App && App->map)
        {
            mapWidth = (float)App->map->mapData.width;
            mapHeight = (float)App->map->mapData.height;
        }
        
        // Screen dimensions: 1280x720 pixels
        
        // Center camera on map center
        camera.target = { mapWidth * 0.5f, mapHeight * 0.5f };
        
        // No rotation in full map mode
        camera.rotation = 0.0f;
        
        // Calculate zoom to fit entire map
        // Use the dimension that requires more zoom reduction
        float zoomX = SCREEN_WIDTH / mapWidth;
        float zoomY = SCREEN_HEIGHT / mapHeight;
        camera.zoom = (zoomX < zoomY) ? zoomX : zoomY;
        
        // Add some margin (95% of calculated zoom)
        camera.zoom *= 0.95f;
    }
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