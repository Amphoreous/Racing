#include "entities/NPCManager.h"
#include "core/Application.h"
#include "core/Map.h"
#include "entities/Car.h"
#include "modules/ModuleResources.h"
#include "raylib.h"

NPCManager::NPCManager(Application* app, bool start_enabled)
	: Module(app, start_enabled)
{
}

NPCManager::~NPCManager()
{
}

bool NPCManager::Start()
{
	LOG("Creating NPC cars");

	// Create the three NPC cars
	CreateNPC("NPC1", "assets/sprites/npc_1.png");
	CreateNPC("NPC2", "assets/sprites/npc_2.png");
	CreateNPC("NPC3", "assets/sprites/npc_3.png");

	LOG("NPC Manager initialized - %d NPCs created", (int)npcCars.size());
	return true;
}

update_status NPCManager::Update()
{
	// Update all NPC cars
	for (Car* npc : npcCars)
	{
		if (npc)
		{
			npc->Update();
		}
	}

	return UPDATE_CONTINUE;
}

update_status NPCManager::PostUpdate()
{
	// Draw all NPC cars
	for (Car* npc : npcCars)
	{
		if (npc)
		{
			npc->Draw();
		}
	}

	return UPDATE_CONTINUE;
}

bool NPCManager::CleanUp()
{
	LOG("Cleaning up NPC Manager");

	// Cleanup all NPC cars
	for (Car* npc : npcCars)
	{
		if (npc)
		{
			delete npc;
		}
	}
	npcCars.clear();

	return true;
}

void NPCManager::CreateNPC(const char* npcName, const char* texturePath)
{
	LOG("Creating NPC: %s", npcName);

	// Create the NPC car
	Car* npcCar = new Car(App);
	if (!npcCar->Start())
	{
		LOG("ERROR: Failed to create NPC car: %s", npcName);
		delete npcCar;
		return;
	}

	// CRITICAL: Get starting position from Tiled map
	// The "Start" objects with Name="NPC1/2/3" are in the "Positions" layer
	// Layer offset: offsetx="1664" offsety="984"

	// Find the Start object with Name property matching this NPC
	MapObject* startPos = nullptr;
	for (const auto& object : App->map->mapData.objects)
	{
		if (object->name == "Start")
		{
			// Check if this object has the "Name" property set to this NPC name
			Properties::Property* nameProp = object->properties.GetProperty("Name");
			if (nameProp && nameProp->value == npcName)
			{
				startPos = object;
				break;
			}
		}
	}

	if (startPos)
	{
		// CRITICAL FIX: The "Positions" layer in Tiled has an offset!
		// offsetx="1664" offsety="984" (from Map.tmx)
		// We need to ADD this offset to the object's position
		const float POSITIONS_LAYER_OFFSET_X = 1664.0f;
		const float POSITIONS_LAYER_OFFSET_Y = 984.0f;

		// Apply layer offset to get world coordinates
		float worldX = (float)startPos->x + POSITIONS_LAYER_OFFSET_X;
		float worldY = (float)startPos->y + POSITIONS_LAYER_OFFSET_Y;

		LOG("Start position found for %s at Tiled coords: (%d, %d)", npcName, startPos->x, startPos->y);
		LOG("With layer offset applied: (%.2f, %.2f)", worldX, worldY);

		// Set NPC car position
		npcCar->SetPosition(worldX, worldY);

		// Set starting rotation (same as player: 270° = facing down)
		npcCar->SetRotation(270.0f);

		LOG("NPC car %s positioned at (%.2f, %.2f) with rotation 270°", npcName, worldX, worldY);
	}
	else
	{
		LOG("Warning: No start position found for %s in map, using default", npcName);
		// Fallback to default position (offset to avoid overlap)
		float defaultX = 500.0f + ((float)npcCars.size() * 100.0f);
		npcCar->SetPosition(defaultX, 300.0f);
		npcCar->SetRotation(270.0f);
	}

	// Configure NPC car with IDENTICAL properties to player
	// CRITICAL: Same dimensions, speed, and capabilities as player car
	npcCar->SetMaxSpeed(800.0f);          // Same as player
	npcCar->SetReverseSpeed(400.0f);      // Same as player
	npcCar->SetAcceleration(20.0f);       // Default acceleration
	npcCar->SetSteeringSensitivity(200.0f); // Default steering

	// Load NPC texture
	Texture2D npcTexture = App->resources->LoadTexture(texturePath);
	if (npcTexture.id != 0)
	{
		npcCar->SetTexture(npcTexture);
		LOG("NPC %s texture loaded: %s", npcName, texturePath);
	}
	else
	{
		LOG("WARNING: Failed to load NPC %s texture: %s", npcName, texturePath);
		// Set a distinct color as fallback for each NPC
		if (strcmp(npcName, "NPC1") == 0)
			npcCar->SetColor(RED);
		else if (strcmp(npcName, "NPC2") == 0)
			npcCar->SetColor(GREEN);
		else if (strcmp(npcName, "NPC3") == 0)
			npcCar->SetColor(YELLOW);
	}

	// Add to NPC list
	npcCars.push_back(npcCar);

	LOG("NPC car %s created successfully", npcName);
}

Car* NPCManager::GetNPC(int index) const
{
	if (index >= 0 && index < (int)npcCars.size())
		return npcCars[index];
	return nullptr;
}