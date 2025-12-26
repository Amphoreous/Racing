#include "core/Globals.h"
#include "core/Application.h"
#include "modules/ModuleRender.h"
#include "modules/ModuleGame.h"
#include "modules/ModuleAudio.h"
#include "modules/ModulePhysics.h"
#include "modules/ModuleResources.h"

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

	// Example 1: Load a background texture (if file exists)
	// Texture2D background = App->resources->LoadTexture("assets/background.png");
	// if (background.id != 0)
	// {
	//	gameElements.emplace_back("background", background, 0, 0);
	//	LOG("Background texture loaded successfully");
	// }

	// Example 2: Load multiple textures
	// const char* textureNames[] = { "player.png", "enemy.png", "obstacle.png" };
	// for (const char* textureName : textureNames)
	// {
	//	std::string fullPath = "assets/";
	//	fullPath += textureName;
	//	Texture2D tex = App->resources->LoadTexture(fullPath.c_str());
	//	if (tex.id != 0)
	//	{
	//		GameElement element(textureName, tex, 100, 100);
	//		gameElements.push_back(element);
	//		LOG("Loaded texture: %s", textureName);
	//	}
	// }

	// NOTE: Best practices:
	// 1. ALL texture loading should use App->resources->LoadTexture()
	// 2. NEVER call ::LoadTexture() directly in ModuleGame
	// 3. The resource manager handles caching and prevents duplicate loads
	// 4. Resources are automatically cleaned up in ModuleResources::CleanUp()
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
	// Render all game elements
	RenderGameElements();

	// TODO: Update track, obstacles, AI opponents, etc.
	// Player car is updated in ModulePlayer

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