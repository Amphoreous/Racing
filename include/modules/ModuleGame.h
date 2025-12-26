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

	bool Start();
	update_status Update();
	bool CleanUp();

	// Resource access (for demonstration purposes)
	int GetLoadedResourceCount() const;
	void PrintResourceStatistics() const;

private:
	// Game elements stored with their textures managed by ModuleResources
	std::vector<GameElement> gameElements;
	
	// Helper method to load game textures
	void LoadGameTextures();
	void RenderGameElements() const;
};
