#include "core/Globals.h"
#include "core/Application.h"
#include "modules/ModuleRender.h"
#include "modules/ModuleGame.h"
#include "modules/ModuleAudio.h"
#include "modules/ModulePhysics.h"
#include "modules/ModuleResources.h"
#include "entities/Player.h"
#include "entities/Car.h"

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

	// Future: Load race track, obstacles, AI cars, etc.
	// For now, player car is handled by ModulePlayer

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

	// Example 2: Load HUD elements
	// Texture2D speedometer = App->resources->LoadTexture("assets/ui/hud/hud_speedometer.png");
	// Texture2D lapCounter = App->resources->LoadTexture("assets/ui/hud/hud_lap_counter.png");
	// if (speedometer.id != 0 && lapCounter.id != 0)
	// {
	//	gameElements.emplace_back("speedometer", speedometer, 50, 50);
	//	gameElements.emplace_back("lap_counter", lapCounter, 800, 50);
	//	LOG("HUD elements loaded successfully");
	// }

	// Example 3: Load game sprites
	// Texture2D playerCar = App->resources->LoadTexture("assets/sprites/car_player.png");
	// if (playerCar.id != 0)
	// {
	//	gameElements.emplace_back("player_car", playerCar, 400, 300);
	//	LOG("Player car sprite loaded successfully");
	// }

	// NOTE: Best practices:
	// 1. ALL texture loading should use App->resources->LoadTexture()
	// 2. NEVER call ::LoadTexture() directly in ModuleGame
	// 3. The resource manager handles caching and prevents duplicate loads
	// 4. Resources are automatically cleaned up in ModuleResources::CleanUp()
}

// Render the background within camera space so it follows the camera
void ModuleGame::RenderTiledBackground() const
{
	if (backgroundTexture.id == 0)
		return; // No background texture loaded

	// Get camera position to center the background on the camera
	float cameraX = 0, cameraY = 0;
	if (App && App->player && App->player->GetCar())
	{
		App->player->GetCar()->GetPosition(cameraX, cameraY);
	}

	// Draw the background texture centered on the camera position
	Rectangle sourceRect = { 0, 0, (float)backgroundTexture.width, (float)backgroundTexture.height };
	Rectangle destRect = { cameraX - backgroundTexture.width/2.0f, cameraY - backgroundTexture.height/2.0f, (float)backgroundTexture.width, (float)backgroundTexture.height };
	Vector2 origin = { 0, 0 };

	DrawTexturePro(backgroundTexture, sourceRect, destRect, origin, 0.0f, WHITE);
}

// Render all game elements
void ModuleGame::RenderGameElements() const
{
	for (const auto& element : gameElements)
	{
		// Example rendering code - adjust based on your needs
		// Vector2 position = { element.position.x, element.position.y };
		// DrawTextureEx(element.texture, position, element.rotation, 1.0f, element.tint);
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