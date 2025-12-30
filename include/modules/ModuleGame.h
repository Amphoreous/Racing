#pragma once

#include "core/Globals.h"
#include "core/Module.h"

#include "core/p2Point.h"

#include "raylib.h"
#include <vector>
#include <map>
#include <string>

class PhysBody;
class PhysicEntity;

// GameElement: Encapsulates a drawable game object with its resources
struct GameElement
{
	std::string name;
	Texture2D texture;
	Vector2 position;
	float rotation;
	Color tint;

	GameElement(const std::string& n, Texture2D tex, float x = 0, float y = 0)
		: name(n), texture(tex), position({ x, y }), rotation(0.0f), tint(WHITE)
	{
	}
};

class ModuleGame : public Module
{
public:
	ModuleGame(Application* app, bool start_enabled = true);
	~ModuleGame();

	bool Start() override;
	update_status Update() override;
	update_status PostUpdate() override;
	bool CleanUp() override;

	// Resource access (for demonstration purposes)
	int GetLoadedResourceCount() const;
	void PrintResourceStatistics() const;

	// Render the tiled background across the entire map
	// screenSpace: true = render centered on screen, false = render centered on camera in world space
	void RenderTiledBackground(bool screenSpace = false) const;

	// Render in-game HUD (speedometer, lap counter)
	void DrawHUD() const;

private:
	// Game elements stored with their textures managed by ModuleResources
	std::vector<GameElement> gameElements;
	
	// Background texture for tiled rendering
	Texture2D backgroundTexture;

	// HUD textures
	Texture2D speedometerTexture;
	Texture2D speedometerNeedleTexture;
	Texture2D lapCounterTexture;
	
	// Helper method to load game textures
	void LoadGameTextures();
	
	// Render all game elements
	void RenderGameElements() const;
};
