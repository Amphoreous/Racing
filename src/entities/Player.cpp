#include "entities/Player.h"
#include "core/Application.h"
#include "entities/Car.h"
#include "raylib.h"

ModulePlayer::ModulePlayer(Application* app, bool start_enabled)
	: Module(app, start_enabled)
	, playerCar(nullptr)
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

	// Configure player car
	playerCar->SetColor(BLUE);
	playerCar->SetMaxSpeed(800.0f);
	playerCar->SetReverseSpeed(400.0f);  // Set reverse speed

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

	return UPDATE_CONTINUE;
}

update_status ModulePlayer::PostUpdate()
{
	if (!playerCar)
		return UPDATE_CONTINUE;

	// Draw the car (after BeginDrawing)
	playerCar->Draw();

	return UPDATE_CONTINUE;
}

bool ModulePlayer::CleanUp()
{
	LOG("Cleaning up player module");

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
	// Reverse (changed from Brake to Reverse!)
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

	// Drift
	if (IsKeyDown(KEY_SPACE))
	{
		playerCar->Drift();
	}
}