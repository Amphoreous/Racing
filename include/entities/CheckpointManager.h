#pragma once

#include "core/Module.h"
#include "entities/PhysBody.h"
#include "raylib.h"
#include <vector>
#include <string>

// Forward declarations
class PhysBody;
class MapObject;

// Checkpoint structure
struct Checkpoint
{
	int order;              // Checkpoint order (0 = finish line, 1-5 = checkpoints)
	std::string name;       // Checkpoint name
	PhysBody* sensor;       // Physics sensor for detection
	bool crossed;           // Has the player crossed this checkpoint?
};

// CheckpointManager: Manages race checkpoints and lap completion
class CheckpointManager : public Module, public PhysBody::CollisionListener
{
public:
	CheckpointManager(Application* app, bool start_enabled = true);
	~CheckpointManager();

	bool Start() override;
	update_status Update() override;
	update_status PostUpdate() override;
	bool CleanUp() override;

	// Collision callbacks
	void OnCollisionEnter(PhysBody* other) override;
	void OnCollisionExit(PhysBody* other) override;

	// Getters
	int GetCurrentLap() const { return currentLap; }
	int GetTotalLaps() const { return totalLaps; }
	int GetTotalCheckpoints() const { return totalCheckpoints; }
	int GetNextCheckpointOrder() const { return nextCheckpointOrder; }
	bool IsRaceFinished() const { return raceFinished; }
	bool IsLapComplete() const;
	int GetCrossedCheckpointsCount() const;
	bool GetCheckpointPosition(int order, float& x, float& y) const;

	// Win screen rendering (called from ModuleRender in screen space)
	void DrawWinScreen();

private:
	// Checkpoint data
	std::vector<Checkpoint> checkpoints;
	Checkpoint* finishLine;

	// Race state
	int currentLap;
	int nextCheckpointOrder;
	int totalCheckpoints;
	int totalLaps;
	bool raceFinished;

	// Player reference
	PhysBody* playerBody;

	// Sound effects
	unsigned int lapCompleteSfxId;

	// Win screen
	Texture2D winBackground;
	bool winBackgroundLoaded;

	// Helper methods
	void LoadCheckpointsFromMap();
	void CreateCheckpointSensor(MapObject* object, int order);
	Checkpoint* FindCheckpointBySensor(PhysBody* sensor);
	bool ValidateCheckpointSequence(int checkpointOrder);
	void ResetCheckpoints();
};