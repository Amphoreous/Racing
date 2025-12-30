#pragma once
#include "core/Module.h"
#include "core/Globals.h"
#include "core/p2Point.h"
#include "raylib.h"

class Car;
class PushAbility;

class ModulePlayer : public Module
{
public:
	ModulePlayer(Application* app, bool start_enabled = true);
	virtual ~ModulePlayer();

	bool Start();
	update_status Update();
	update_status PostUpdate();
	bool CleanUp();

	// Access to player car
	Car* GetCar() const { return playerCar; }

	// Access to push ability
	PushAbility* GetAbility() const { return pushAbility; }

private:
	Car* playerCar;
	PushAbility* pushAbility;

	// Car passing sound
	Music carPassingSound;
	bool isCarPassingPlaying;

	void HandleInput();
	void CheckNPCPassing();  // NEW: Check if NPCs are passing
};