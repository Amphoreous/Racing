#include "entities/CheckpointManager.h"
#include "core/Application.h"
#include "core/Map.h"
#include "modules/ModulePhysics.h"
#include "modules/ModuleAudio.h"
#include "entities/Player.h"
#include "entities/Car.h"
#include "raylib.h"
#include <algorithm>

CheckpointManager::CheckpointManager(Application* app, bool start_enabled)
	: Module(app, start_enabled)
	, finishLine(nullptr)
	, currentLap(1)
	, nextCheckpointOrder(1)
	, totalCheckpoints(5)
	, totalLaps(5)
	, raceFinished(false)
	, playerBody(nullptr)
	, lapCompleteSfxId(0)
{
}

CheckpointManager::~CheckpointManager()
{
}

bool CheckpointManager::Start()
{
	LOG("Initializing Checkpoint Manager");

	if (App->player && App->player->GetCar())
	{
		playerBody = App->player->GetCar()->GetPhysBody();
	}

	if (!playerBody)
	{
		LOG("ERROR: CheckpointManager - Player body not found!");
		return false;
	}

	LoadCheckpointsFromMap();

	// Load checkpoint.wav
	if (App->audio)
	{
		lapCompleteSfxId = App->audio->LoadFx("assets/audio/fx/checkpoint.wav");
	}

	LOG("CheckpointManager initialized - %d checkpoints loaded", (int)checkpoints.size());
	LOG("Race configuration: %d laps, %d checkpoints per lap", totalLaps, totalCheckpoints);
	LOG("Current lap: %d, Next checkpoint: %d", currentLap, nextCheckpointOrder);

	return true;
}

update_status CheckpointManager::Update()
{
	if (raceFinished)
	{
		LOG("=== RACE FINISHED! ===");
		LOG("Player completed %d laps!", totalLaps);
		return UPDATE_STOP;
	}

	return UPDATE_CONTINUE;
}

update_status CheckpointManager::PostUpdate()
{
	if (raceFinished)
	{
		DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.7f));

		const char* victoryText = "RACE COMPLETE!";
		const char* lapsText = TextFormat("Completed %d laps!", totalLaps);
		const char* exitText = "Press ESC to exit";

		int titleFontSize = 60;
		int subFontSize = 30;
		int exitFontSize = 20;

		int titleWidth = MeasureText(victoryText, titleFontSize);
		int lapsWidth = MeasureText(lapsText, subFontSize);
		int exitWidth = MeasureText(exitText, exitFontSize);

		DrawText(victoryText,
			SCREEN_WIDTH / 2 - titleWidth / 2,
			SCREEN_HEIGHT / 2 - 60,
			titleFontSize,
			GOLD);

		DrawText(lapsText,
			SCREEN_WIDTH / 2 - lapsWidth / 2,
			SCREEN_HEIGHT / 2 + 20,
			subFontSize,
			WHITE);

		DrawText(exitText,
			SCREEN_WIDTH / 2 - exitWidth / 2,
			SCREEN_HEIGHT / 2 + 80,
			exitFontSize,
			GRAY);
	}

	return UPDATE_CONTINUE;
}

bool CheckpointManager::CleanUp()
{
	LOG("Cleaning up Checkpoint Manager");

	checkpoints.clear();
	finishLine = nullptr;
	playerBody = nullptr;

	return true;
}

void CheckpointManager::LoadCheckpointsFromMap()
{
	if (!App->map)
	{
		LOG("ERROR: CheckpointManager - Map module not available");
		return;
	}

	LOG("=== CHECKPOINT LOADING DEBUG ===");
	LOG("Total map objects found: %d", (int)App->map->mapData.objects.size());

	int checkpointCount = 0;
	for (const auto& object : App->map->mapData.objects)
	{
		LOG("Object found: name='%s', type='%s', at (%d,%d) size (%d,%d)",
			object->name.c_str(),
			object->type.c_str(),
			object->x, object->y,
			object->width, object->height);

		LOG("  Properties count: %d", (int)object->properties.propertyList.size());
		for (const auto& prop : object->properties.propertyList)
		{
			LOG("    Property: %s = %s", prop->name.c_str(), prop->value.c_str());
		}

		Properties::Property* orderProp = object->properties.GetProperty("Order");

		if (!orderProp)
		{
			LOG("  -> Skipped (no Order property)");
			continue;
		}

		checkpointCount++;
		LOG("  -> CHECKPOINT DETECTED (has Order property)!");

		int order = std::stoi(orderProp->value);
		LOG("  -> Order value: %d", order);

		CreateCheckpointSensor(object, order);

		LOG("Loaded checkpoint: %s (Order: %d) at (%d, %d)",
			object->name.c_str(), order, object->x, object->y);
	}

	LOG("Total checkpoints processed: %d", checkpointCount);

	for (auto& checkpoint : checkpoints)
	{
		if (checkpoint.order == 0)
		{
			finishLine = &checkpoint;
			LOG("Finish line found: %s", checkpoint.name.c_str());
			break;
		}
	}

	if (!finishLine)
	{
		LOG("WARNING: No finish line (checkpoint with order=0) found!");
	}

	LOG("=== CHECKPOINT LOADING COMPLETE ===");
}

void CheckpointManager::CreateCheckpointSensor(MapObject* object, int order)
{
	if (!App->physics || object->width <= 0 || object->height <= 0)
		return;

	LOG("=== CREATING SENSOR FOR %s ===", object->name.c_str());
	LOG("  Tiled coords (raw): X=%d Y=%d W=%d H=%d", object->x, object->y, object->width, object->height);

	const float CHECKPOINTS_LAYER_OFFSET_X = 32.0f;
	const float CHECKPOINTS_LAYER_OFFSET_Y = 240.0f;

	float worldX = (float)object->x + CHECKPOINTS_LAYER_OFFSET_X;
	float worldY = (float)object->y + CHECKPOINTS_LAYER_OFFSET_Y;

	LOG("  With layer offset: X=%.2f Y=%.2f", worldX, worldY);

	float centerX = worldX + (float)object->width * 0.5f;
	float centerY = worldY + (float)object->height * 0.5f;

	LOG("  Calculated center (pixels): (%.2f, %.2f)", centerX, centerY);

	PhysBody* sensor = App->physics->CreateRectangle(
		centerX, centerY,
		(float)object->width, (float)object->height,
		PhysBody::BodyType::STATIC
	);

	if (!sensor)
	{
		LOG("ERROR: Failed to create checkpoint sensor for %s", object->name.c_str());
		return;
	}

	sensor->SetSensor(true);

	float verifyX, verifyY;
	sensor->GetPositionF(verifyX, verifyY);
	LOG("  Box2D returned position (pixels): (%.2f, %.2f)", verifyX, verifyY);
	LOG("  Difference: deltaX=%.2f deltaY=%.2f", verifyX - centerX, verifyY - centerY);

	Checkpoint checkpoint;
	checkpoint.order = order;
	checkpoint.name = object->name;
	checkpoint.sensor = sensor;
	checkpoint.crossed = false;

	checkpoints.push_back(checkpoint);

	Checkpoint* stablePtr = &checkpoints.back();

	stablePtr->sensor->SetUserData(stablePtr);
	stablePtr->sensor->SetCollisionListener(this);

	LOG("=== SENSOR CREATED SUCCESSFULLY ===\n");
}

Checkpoint* CheckpointManager::FindCheckpointBySensor(PhysBody* sensor)
{
	for (auto& checkpoint : checkpoints)
	{
		if (checkpoint.sensor == sensor)
			return &checkpoint;
	}
	return nullptr;
}

void CheckpointManager::OnCollisionEnter(PhysBody* other)
{
	if (raceFinished)
		return;

	if (!other || other != playerBody)
		return;

	LOG("Player collision with checkpoint sensor detected!");

	float carX, carY;
	other->GetPositionF(carX, carY);

	Checkpoint* hitCheckpoint = nullptr;
	float minDistance = FLT_MAX;

	for (auto& checkpoint : checkpoints)
	{
		if (!checkpoint.sensor)
			continue;

		float checkX, checkY;
		checkpoint.sensor->GetPositionF(checkX, checkY);

		float dx = carX - checkX;
		float dy = carY - checkY;
		float distance = sqrtf(dx * dx + dy * dy);

		if (distance < minDistance)
		{
			minDistance = distance;
			hitCheckpoint = &checkpoint;
		}
	}

	if (hitCheckpoint)
	{
		LOG("Collided with checkpoint: %s", hitCheckpoint->name.c_str());
		ValidateCheckpointSequence(hitCheckpoint->order);
	}
}

bool CheckpointManager::ValidateCheckpointSequence(int checkpointOrder)
{
	LOG("ValidateCheckpointSequence called with order: %d", checkpointOrder);

	Checkpoint* checkpoint = nullptr;

	for (auto& cp : checkpoints)
	{
		if (cp.order == checkpointOrder)
		{
			checkpoint = &cp;
			break;
		}
	}

	if (!checkpoint)
	{
		LOG("  -> ERROR: Checkpoint with order %d not found!", checkpointOrder);
		return false;
	}

	LOG("  -> Found checkpoint: %s", checkpoint->name.c_str());

	// FINISH LINE logic (order == 0)
	if (checkpoint->order == 0)
	{
		LOG("  -> This is the finish line");

		bool allCheckpointsCrossed = true;
		for (const auto& cp : checkpoints)
		{
			if (cp.order > 0 && !cp.crossed)
			{
				allCheckpointsCrossed = false;
				LOG("  -> Missing checkpoint: %s", cp.name.c_str());
				break;
			}
		}

		if (allCheckpointsCrossed)
		{
			LOG("=== LAP %d COMPLETE! ===", currentLap);

			// Play checkpoint.wav
			if (App->audio && lapCompleteSfxId > 0)
			{
				App->audio->PlayFx(lapCompleteSfxId);
				LOG("Playing lap completion sound");
			}

			if (currentLap >= totalLaps)
			{
				raceFinished = true;
				LOG("╔═══════════════════════════════════╗");
				LOG("║   RACE FINISHED - %d LAPS DONE!   ║", totalLaps);
				LOG("╚═══════════════════════════════════╝");
				return true;
			}

			currentLap++;
			LOG("Starting lap %d / %d", currentLap, totalLaps);

			ResetCheckpoints();
			nextCheckpointOrder = 1;

			return true;
		}
		else
		{
			LOG("Cannot cross finish line - missing checkpoints!");
			return false;
		}
	}

	// CHECKPOINT logic (order 1-5)
	LOG("  -> Expected next checkpoint: %d", nextCheckpointOrder);

	if (checkpoint->order == nextCheckpointOrder)
	{
		if (!checkpoint->crossed)
		{
			checkpoint->crossed = true;
			LOG("=== Checkpoint %s crossed! (%d/%d) ===",
				checkpoint->name.c_str(), checkpoint->order, totalCheckpoints);

			nextCheckpointOrder++;

			if (nextCheckpointOrder > totalCheckpoints)
			{
				nextCheckpointOrder = 0;
				LOG("All checkpoints crossed - head to finish line!");
			}

			return true;
		}
		else
		{
			LOG("  -> Checkpoint already crossed");
		}
	}
	else
	{
		LOG("=== Checkpoint %s crossed OUT OF ORDER (expected: %d, got: %d) ===",
			checkpoint->name.c_str(), nextCheckpointOrder, checkpoint->order);
		return false;
	}

	return false;
}

void CheckpointManager::ResetCheckpoints()
{
	LOG("Resetting all checkpoints");
	for (auto& checkpoint : checkpoints)
	{
		checkpoint.crossed = false;
	}
}

void CheckpointManager::OnCollisionExit(PhysBody* other)
{
	// Not used for checkpoints
}

bool CheckpointManager::IsLapComplete() const
{
	for (const auto& cp : checkpoints)
	{
		if (cp.order > 0 && !cp.crossed)
			return false;
	}
	return true;
}

int CheckpointManager::GetCrossedCheckpointsCount() const
{
	int count = 0;
	for (const auto& cp : checkpoints)
	{
		if (cp.order > 0 && cp.crossed)
			count++;
	}
	return count;
}

bool CheckpointManager::GetCheckpointPosition(int order, float& x, float& y) const
{
	for (const auto& cp : checkpoints)
	{
		if (cp.order == order && cp.sensor)
		{
			cp.sensor->GetPositionF(x, y);
			return true;
		}
	}
	return false;
}