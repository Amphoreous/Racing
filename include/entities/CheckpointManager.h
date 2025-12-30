#pragma once

#include "core/Module.h"
#include "core/Globals.h"
#include "entities/PhysBody.h"
#include <vector>
#include <string>

// Forward declarations
class MapObject;

// Checkpoint: Represents a single checkpoint sensor
struct Checkpoint
{
	int order;                  // Order in sequence (1-5 for C1-C5, 0 for FL)
	std::string name;           // Name from Tiled (e.g., "C1", "FL")
	PhysBody* sensor;           // Physics sensor body
	bool crossed;               // Has player crossed this checkpoint?

	Checkpoint() : order(0), name(""), sensor(nullptr), crossed(false) {}
};

// CheckpointManager: Manages race progress tracking via checkpoint sensors
class CheckpointManager : public Module, public PhysBody::CollisionListener
{
public:
	CheckpointManager(Application* app, bool start_enabled = true);
	virtual ~CheckpointManager();

	bool Start() override;
	update_status Update() override;
	update_status PostUpdate() override;
	bool CleanUp() override;

	// Collision listener interface
	void OnCollisionEnter(PhysBody* other) override;
	void OnCollisionExit(PhysBody* other) override;

	// Lap management
	int GetCurrentLap() const { return currentLap; }
	int GetNextCheckpointOrder() const { return nextCheckpointOrder; }
	int GetTotalCheckpoints() const { return totalCheckpoints; }
	int GetCrossedCheckpointsCount() const;
	bool IsLapComplete() const;

	// Race completion
	bool IsRaceFinished() const { return raceFinished; }
	int GetTotalLaps() const { return totalLaps; }
	
private:
	std::vector<Checkpoint> checkpoints;  // All checkpoints including finish line
	Checkpoint* finishLine;                // Quick reference to finish line

	int currentLap;                        // Current lap number (starts at 1)
	int nextCheckpointOrder;               // Next checkpoint to cross (1-5, or 0 for FL)
	int totalCheckpoints;                  // Total ordered checkpoints (5 in C1-C5)
	int totalLaps;                         // Total laps to complete the race (5)
	bool raceFinished;                     // Flag indicating race is complete

	PhysBody* playerBody;                  // Reference to player car body

	// Helper methods
	void LoadCheckpointsFromMap();
	void CreateCheckpointSensor(MapObject* object, int order);
	Checkpoint* FindCheckpointBySensor(PhysBody* sensor);
	void ResetCheckpoints();
	bool ValidateCheckpointSequence(int checkpointOrder);
};