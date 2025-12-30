#include "core/Globals.h"
#include "core/Application.h"
#include "modules/ModuleRender.h"
#include "modules/ModuleGame.h"
#include "modules/ModuleAudio.h"
#include "modules/ModulePhysics.h"
#include "modules/ModuleResources.h"
#include "entities/Player.h"
#include "entities/Car.h"
#include "entities/CheckpointManager.h"

ModuleGame::ModuleGame(Application* app, bool start_enabled) : Module(app, start_enabled)
{
}

ModuleGame::~ModuleGame()
{
}

// Load assets
bool ModuleGame::Start()
{
	LOG("Loading Game assets");
	bool ret = true;

	// Load all game textures through the resource manager
	LoadGameTextures();

	// Print resource statistics after loading
	PrintResourceStatistics();

	return ret;
}

// Load textures through the centralized resource manager
void ModuleGame::LoadGameTextures()
{
	LOG("Loading game textures through resource manager...");

	backgroundTexture = App->resources->LoadTexture("assets/ui/backgrounds/main_background.jpg");
	if (backgroundTexture.id != 0)
	{
		LOG("Main background loaded successfully for background rendering");
	}
	else
	{
		LOG("Failed to load main background texture!");
	}

	// Load HUD elements
	speedometerTexture = App->resources->LoadTexture("assets/ui/hud/hud_speedometer.png");
	speedometerNeedleTexture = App->resources->LoadTexture("assets/ui/hud/hud_speedometer_direction.png");
	lapCounterTexture = App->resources->LoadTexture("assets/ui/hud/hud_lap_counter.png");
	
	if (speedometerTexture.id != 0 && speedometerNeedleTexture.id != 0 && lapCounterTexture.id != 0)
	{
		LOG("HUD elements loaded successfully");
	}
	else
	{
		LOG("Warning: Some HUD elements failed to load");
	}
}

// Render the background within camera space so it follows the camera
void ModuleGame::RenderTiledBackground(bool screenSpace) const
{
	if (backgroundTexture.id == 0)
		return; // No background texture loaded

	if (screenSpace)
	{
		// Render centered on screen (for full map view)
		Rectangle sourceRect = { 0, 0, (float)backgroundTexture.width, (float)backgroundTexture.height };
		Rectangle destRect = { SCREEN_WIDTH/2.0f - backgroundTexture.width/2.0f, SCREEN_HEIGHT/2.0f - backgroundTexture.height/2.0f, (float)backgroundTexture.width, (float)backgroundTexture.height };
		Vector2 origin = { 0, 0 };
		DrawTexturePro(backgroundTexture, sourceRect, destRect, origin, 0.0f, WHITE);
	}
	else
	{
		// Render in world space centered on camera (for follow car modes)
		float cameraX = 0, cameraY = 0;
		if (App && App->player && App->player->GetCar())
		{
			App->player->GetCar()->GetPosition(cameraX, cameraY);
		}

		Rectangle sourceRect = { 0, 0, (float)backgroundTexture.width, (float)backgroundTexture.height };
		Rectangle destRect = { cameraX - backgroundTexture.width/2.0f, cameraY - backgroundTexture.height/2.0f, (float)backgroundTexture.width, (float)backgroundTexture.height };
		Vector2 origin = { 0, 0 };
		DrawTexturePro(backgroundTexture, sourceRect, destRect, origin, 0.0f, WHITE);
	}
}

// Cleanup assets
bool ModuleGame::CleanUp()
{
	LOG("Unloading Game scene");

	// Clear game elements
	gameElements.clear();

	// All resource cleanup is handled automatically by ModuleResources::CleanUp()
	// which is called after this module's CleanUp()
	LOG("Game scene resources released (manager will handle texture unloading)");

	return true;
}

// Update game logic
update_status ModuleGame::Update()
{
	// Game logic updates happen here
	// Player car is updated in ModulePlayer

	return UPDATE_CONTINUE;
}

// Post-update: Render everything (called after BeginDrawing)
update_status ModuleGame::PostUpdate()
{
	RenderGameElements();

	return UPDATE_CONTINUE;
}

// Render all game elements
void ModuleGame::RenderGameElements() const
{
	for (const auto& element : gameElements)
	{
		// Example rendering code
		// Vector2 position = { element.position.x, element.position.y };
		// DrawTextureEx(element.texture, position, element.rotation, 1.0f, element.tint);
	}
}

// Get total loaded resources count
int ModuleGame::GetLoadedResourceCount() const
{
	int total = 0;
	total += App->resources->GetTextureCount();
	total += App->resources->GetSoundCount();
	total += App->resources->GetMusicCount();
	return total;
}

// Print resource statistics for debugging
void ModuleGame::PrintResourceStatistics() const
{
	LOG("Resource statistics:");
	LOG("Textures loaded: %d", App->resources->GetTextureCount());
	LOG("Sounds loaded: %d", App->resources->GetSoundCount());
	LOG("Music tracks loaded: %d", App->resources->GetMusicCount());
	LOG("Total resources: %d", GetLoadedResourceCount());
}

// Draw in-game HUD (speedometer and lap counter)
void ModuleGame::DrawHUD() const
{
	// Only draw HUD during race running state
	if (!App->checkpointManager || App->checkpointManager->GetRaceState() != RACE_RUNNING)
		return;

	// Lap Counter
	// Position: top-left corner
	float lapScale = 0.3f;
	float lapX = 20.0f;
	float lapY = 20.0f;

	Rectangle lapSource = { 0, 0, (float)lapCounterTexture.width, (float)lapCounterTexture.height };
	Rectangle lapDest = {
		lapX,
		lapY,
		lapCounterTexture.width * lapScale,
		lapCounterTexture.height * lapScale
	};
	DrawTexturePro(lapCounterTexture, lapSource, lapDest, {0, 0}, 0.0f, WHITE);

	// Draw lap text centered inside the visible lap counter box
	int currentLap = App->checkpointManager->GetCurrentLap();
	int totalLaps = App->checkpointManager->GetTotalLaps();
	
	const char* lapText = TextFormat("%d / %d", currentLap, totalLaps);
	int lapFontSize = 24;  // Fixed font size
	int lapTextWidth = MeasureText(lapText, lapFontSize);
	
	// The visible blue box is at the TOP-LEFT of the texture
	// Based on screenshot: box starts near top and is roughly 40% width, 20% height
	float scaledWidth = lapCounterTexture.width * lapScale;
	float scaledHeight = lapCounterTexture.height * lapScale;
	
	// The blue box appears to be from X=0 to ~40% width, Y=5% to ~25% height
	float visibleBoxX = lapX;
	float visibleBoxY = lapY + (scaledHeight * 0.05f);
	float visibleBoxWidth = scaledWidth * 0.40f;
	float visibleBoxHeight = scaledHeight * 0.20f;
	
	float lapTextX = visibleBoxX + (visibleBoxWidth / 2.0f) - (lapTextWidth / 2.0f);
	float lapTextY = visibleBoxY + (visibleBoxHeight / 2.0f) - (lapFontSize / 2.0f);
	
	// Draw shadow then text
	DrawText(lapText, (int)lapTextX + 2, (int)lapTextY + 2, lapFontSize, BLACK);
	DrawText(lapText, (int)lapTextX, (int)lapTextY, lapFontSize, WHITE);

	// === SPEEDOMETER ===
	// Position: bottom-right corner with padding
	float speedometerScale = 0.15f;  // Small speedometer
	float speedometerWidth = speedometerTexture.width * speedometerScale;
	float speedometerHeight = speedometerTexture.height * speedometerScale;
	float speedometerX = SCREEN_WIDTH - speedometerWidth - 20.0f;
	float speedometerY = SCREEN_HEIGHT - speedometerHeight - 20.0f;

	// Draw speedometer background
	Rectangle speedometerSource = { 0, 0, (float)speedometerTexture.width, (float)speedometerTexture.height };
	Rectangle speedometerDest = { 
		speedometerX, 
		speedometerY, 
		speedometerWidth, 
		speedometerHeight 
	};
	DrawTexturePro(speedometerTexture, speedometerSource, speedometerDest, {0, 0}, 0.0f, WHITE);

	// Get current speed from player car
	float currentSpeed = 0.0f;
	float maxCarSpeed = 1100.0f;  // Max speed from Player.cpp
	if (App->player && App->player->GetCar())
	{
		currentSpeed = App->player->GetCar()->GetCurrentSpeed();
	}

	// Convert physics units to km/h (0-100 range for display)
	// Max physics speed is 1100, so normalize to 0-100 km/h display
	float speedKmh = (currentSpeed / maxCarSpeed) * 100.0f;
	speedKmh = fminf(fmaxf(speedKmh, 0.0f), 100.0f);  // Clamp to 0-100

	// Calculate needle rotation
	// 0 km/h = -90 degrees (pointing left/down)
	// 100 km/h = +90 degrees (pointing right)
	// Range is 180 degrees for 0-100 km/h
	float needleAngle = -90.0f + (speedKmh / 100.0f) * 180.0f;

	// Draw needle on the speedometer (same size as speedometer)
	// Both images are 1890x1417 pixels
	Rectangle needleSource = { 0, 0, (float)speedometerNeedleTexture.width, (float)speedometerNeedleTexture.height };
	
	// The pivot point is at center X (50%) and 75% down in the image
	// We need to place this pivot at the same screen position as the speedometer's pivot
	float pivotScreenX = speedometerX + speedometerWidth / 2.0f;
	float pivotScreenY = speedometerY + speedometerHeight * 0.75f;
	
	Rectangle needleDest = {
		pivotScreenX,
		pivotScreenY,
		speedometerWidth,
		speedometerHeight
	};
	
	// Origin: where the pivot is within the needle texture (center X, 75% Y)
	Vector2 needleOrigin = {
		speedometerWidth / 2.0f,
		speedometerHeight * 0.75f
	};
	DrawTexturePro(speedometerNeedleTexture, needleSource, needleDest, needleOrigin, needleAngle, WHITE);
}