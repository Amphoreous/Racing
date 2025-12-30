#pragma once

#include "core/Module.h"
#include "entities/PhysBody.h"
#include "raylib.h"
#include <vector>
#include <string>

// Forward declarations
class PhysBody;
class MapObject;

// Race state enumeration
enum RaceState
{
	RACE_GET_READY,  // "GET READY!" pause before intro
	RACE_INTRO,      // Camera panning around track
	RACE_COUNTDOWN,  // 3, 2, 1, GO!
	RACE_RUNNING,    // Race in progress
	RACE_FINISHED    // Player won
};

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

	// Race state
	RaceState GetRaceState() const { return raceState; }
	bool CanPlayerMove() const { return raceState == RACE_RUNNING; }
	float GetCountdownValue() const { return countdownTimer; }

	// Win screen rendering (called from ModuleRender in screen space)
	void DrawWinScreen();
	void DrawCountdown();

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
	
	// Intro and countdown
	RaceState raceState;
	float getReadyTimer;     // Timer for GET READY pause
	float getReadyDuration;  // Duration of GET READY pause (3 seconds)
	float introTimer;
	float countdownTimer;
	int lastCountdownNumber;
	float introDuration;
	
	// Intro camera positions
	float introStartX, introStartY;
	float introEndX, introEndY;
	float introEndRotation;  // Player's starting rotation

	// Player reference
	PhysBody* playerBody;

	// Sound effects
	unsigned int lapCompleteSfxId;
	unsigned int countdownBeepSfxId;
	unsigned int countdownGoSfxId;

	// Win screen
	Texture2D winBackground;
	bool winBackgroundLoaded;

	// Helper methods
	void LoadCheckpointsFromMap();
	void CreateCheckpointSensor(MapObject* object, int order);
	Checkpoint* FindCheckpointBySensor(PhysBody* sensor);
	bool ValidateCheckpointSequence(int checkpointOrder);
	void ResetCheckpoints();
	void UpdateGetReady();
	void UpdateIntro();
	void UpdateCountdown();
};