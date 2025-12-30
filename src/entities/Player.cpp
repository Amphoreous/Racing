#include "entities/Player.h"
#include "entities/PushAbility.h"
#include "core/Application.h"
#include "core/Map.h"
#include "entities/Car.h"
#include "entities/NPCManager.h"
#include "modules/ModulePhysics.h"
#include "modules/ModuleAudio.h"
#include "modules/ModuleResources.h"
#include "raylib.h"

ModulePlayer::ModulePlayer(Application* app, bool start_enabled)
	: Module(app, start_enabled)
	, playerCar(nullptr)
	, pushAbility(nullptr)
	, carPassingSfxId(0)
{
}

ModulePlayer::~ModulePlayer()
{
}

bool ModulePlayer::Start()
{
	LOG("Creating player car");

	// Clean up existing resources before creating new ones
	// This prevents memory leaks when Start() is called multiple times (Enable/Disable cycles)
	if (playerCar)
	{
		LOG("Player car already exists - cleaning up before re-creation");
		delete playerCar;
		playerCar = nullptr;
	}
	if (pushAbility)
	{
		delete pushAbility;
		pushAbility = nullptr;
	}

	// Start background music (looping)
	if (App->audio)
	{
		App->audio->PlayMusic("assets/audio/music/music.wav");
	}

	// Create the player's car
	playerCar = new Car(App);
	if (!playerCar->Start())
	{
		LOG("ERROR: Failed to create player car");
		return false;
	}

	// Get starting position from Tiled map
	// The "Start" object with Name="Player" is in the "Positions" layer
	// Layer offset: offsetx="1664" offsety="984"

	// Find the Start object with Name property = "Player"
	MapObject* startPos = nullptr;
	for (const auto& object : App->map->mapData.objects)
	{
		if (object->name == "Start")
		{
			// Check if this object has the "Name" property set to "Player"
			Properties::Property* nameProp = object->properties.GetProperty("Name");
			if (nameProp && nameProp->value == "Player")
			{
				startPos = object;
				break;
			}
		}
	}

	if (startPos)
	{
		// The "Positions" layer in Tiled has an offset!
		// offsetx="1664" offsety="984" (from Map.tmx)
		// We need to ADD this offset to the object's position
		const float POSITIONS_LAYER_OFFSET_X = 1664.0f;
		const float POSITIONS_LAYER_OFFSET_Y = 984.0f;

		// Apply layer offset to get world coordinates
		float worldX = (float)startPos->x + POSITIONS_LAYER_OFFSET_X;
		float worldY = (float)startPos->y + POSITIONS_LAYER_OFFSET_Y;

		LOG("Start position found at Tiled coords: (%d, %d)", startPos->x, startPos->y);
		LOG("With layer offset applied: (%.2f, %.2f)", worldX, worldY);

		// Set player car position
		playerCar->SetPosition(worldX, worldY);

		// Set starting rotation (270° = facing down)
		playerCar->SetRotation(270.0f);

		LOG("Player car positioned at (%.2f, %.2f) with rotation 270°", worldX, worldY);
	}
	else
	{
		LOG("Warning: No start position found for Player in map, using default (400, 300)");
		// Fallback to default position
		playerCar->SetPosition(400.0f, 300.0f);
		playerCar->SetRotation(270.0f);
	}

	// Configure player car
	playerCar->SetMaxSpeed(1100.0f);
	playerCar->SetReverseSpeed(500.0f);

	// Initialize push ability
	pushAbility = new PushAbility();
	if (!pushAbility->Init(App, true))
	{
		LOG("ERROR: Failed to initialize push ability");
		delete pushAbility;
		pushAbility = nullptr;
	}

	// Load car passing sound as Sound effect (not Music)
	if (App->audio)
	{
		carPassingSfxId = App->audio->LoadFx("assets/audio/fx/car_passing.wav");
		if (carPassingSfxId > 0)
		{
			LOG("Car passing sound loaded successfully (ID: %u)", carPassingSfxId);
		}
		else
		{
			LOG("WARNING: Failed to load car passing sound");
		}
	}

	LOG("Player car created successfully");
	return true;
}

update_status ModulePlayer::Update()
{
	if (!playerCar)
		return UPDATE_CONTINUE;

	// Handle player input
	HandleInput();

	// Update car physics
	playerCar->Update();

	// Update push ability
	if (pushAbility)
	{
		pushAbility->Update();
	}

	// Check if NPCs are passing
	CheckNPCPassing();

	return UPDATE_CONTINUE;
}

update_status ModulePlayer::PostUpdate()
{
	if (!playerCar)
		return UPDATE_CONTINUE;

	// Draw the car (after BeginDrawing)
	playerCar->Draw();

	// Draw push ability effect
	if (pushAbility)
	{
		pushAbility->Draw();
	}

	return UPDATE_CONTINUE;
}

bool ModulePlayer::CleanUp()
{
	LOG("Cleaning up player module");

	// Cleanup push ability
	if (pushAbility)
	{
		delete pushAbility;
		pushAbility = nullptr;
	}

	// Cleanup player car
	if (playerCar)
	{
		delete playerCar;
		playerCar = nullptr;
	}

	return true;
}

void ModulePlayer::HandleInput()
{
	if (!playerCar)
		return;

	// Acceleration (forward)
	if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
	{
		playerCar->Accelerate(1.0f);
	}
	// Reverse
	else if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
	{
		playerCar->Reverse(1.0f);
	}

	// Steering
	if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
	{
		playerCar->Steer(-1.0f);
	}
	else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
	{
		playerCar->Steer(1.0f);
	}
	else
	{
		// No steering input - reset angular velocity
		playerCar->Steer(0.0f);
	}

	// PUSH ABILITY (replaces drift)
	if (IsKeyPressed(KEY_SPACE))
	{
		if (pushAbility)
		{
			float playerX, playerY;
			playerCar->GetPosition(playerX, playerY);

			// Get player rotation
			float playerRotation = playerCar->GetRotation();

			// Activate ability with position, rotation AND the car that's using it
			pushAbility->Activate(playerX, playerY, playerRotation, playerCar);
		}
	}
}

void ModulePlayer::CheckNPCPassing()
{
	static bool wasNPCNearby = false;  // Track previous state

	if (!playerCar || !App->npcManager || !App->audio)
	{
		return;
	}

	if (carPassingSfxId == 0)
	{
		return;
	}

	// Get player position
	float playerX, playerY;
	playerCar->GetPosition(playerX, playerY);

	const float PASSING_DISTANCE = 200.0f;  // Distance to detect nearby NPCs
	const float MIN_NPC_SPEED = 15.0f;      // NPC must be moving

	bool npcPassingNearby = false;

	const std::vector<Car*>& npcs = App->npcManager->GetNPCs();

	for (Car* npc : npcs)
	{
		if (!npc)
			continue;

		float npcX, npcY;
		npc->GetPosition(npcX, npcY);
		float npcSpeed = npc->GetCurrentSpeed();

		float dx = npcX - playerX;
		float dy = npcY - playerY;
		float distance = sqrtf(dx * dx + dy * dy);

		if (distance < PASSING_DISTANCE && npcSpeed > MIN_NPC_SPEED)
		{
			npcPassingNearby = true;
			break;
		}
	}

	// Play sound when NPC enters range (edge detection)
	if (npcPassingNearby && !wasNPCNearby)
	{
		App->audio->PlayFx(carPassingSfxId);
	}

	wasNPCNearby = npcPassingNearby;
}