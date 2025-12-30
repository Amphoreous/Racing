#include "entities/Player.h"
#include "entities/PushAbility.h"
#include "core/Application.h"
#include "core/Map.h"
#include "entities/Car.h"
#include "modules/ModulePhysics.h"
#include "raylib.h"

ModulePlayer::ModulePlayer(Application* app, bool start_enabled)
	: Module(app, start_enabled)
	, playerCar(nullptr)
	, pushAbility(nullptr)
{
}

ModulePlayer::~ModulePlayer()
{
}

bool ModulePlayer::Start()
{
	LOG("Creating player car");

	// Create the player's car
	playerCar = new Car(App);
	if (!playerCar->Start())
	{
		LOG("ERROR: Failed to create player car");
		return false;
	}

	// CRITICAL: Get starting position from Tiled map
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
		// CRITICAL FIX: The "Positions" layer in Tiled has an offset!
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
	playerCar->SetColor(BLUE);
	playerCar->SetMaxSpeed(800.0f);
	playerCar->SetReverseSpeed(400.0f);

	// Initialize push ability
	pushAbility = new PushAbility();
	if (!pushAbility->Init(App))
	{
		LOG("ERROR: Failed to initialize push ability");
		delete pushAbility;
		pushAbility = nullptr;
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

	// Draw ability cooldown UI (optional debug)
	if (pushAbility && App->physics && App->physics->IsDebugMode())
	{
		float progress = pushAbility->GetCooldownProgress();
		DrawRectangle(10, 350, 200, 30, Fade(BLACK, 0.8f));
		DrawRectangle(10, 350, (int)(200 * progress), 30, pushAbility->IsReady() ? GREEN : YELLOW);
		DrawText(pushAbility->IsReady() ? "ABILITY READY" : "COOLDOWN", 15, 355, 20, WHITE);
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

			// Activate ability with position AND rotation
			pushAbility->Activate(playerX, playerY, playerRotation);
		}
	}
}