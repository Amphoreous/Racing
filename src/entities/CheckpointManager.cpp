#include "entities/CheckpointManager.h"
#include "core/Application.h"
#include "core/Map.h"
#include "modules/ModulePhysics.h"
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
	, playerBody(nullptr)
{
}

CheckpointManager::~CheckpointManager()
{
}

bool CheckpointManager::Start()
{
	LOG("Initializing Checkpoint Manager");

	// Get player car physics body reference
	if (App->player && App->player->GetCar())
	{
		playerBody = App->player->GetCar()->GetPhysBody();
	}

	if (!playerBody)
	{
		LOG("ERROR: CheckpointManager - Player body not found!");
		return false;
	}

	// Load checkpoints from map
	LoadCheckpointsFromMap();

	LOG("CheckpointManager initialized - %d checkpoints loaded", (int)checkpoints.size());
	LOG("Current lap: %d, Next checkpoint: %d", currentLap, nextCheckpointOrder);

	return true;
}

update_status CheckpointManager::Update()
{
	return UPDATE_CONTINUE;
}

update_status CheckpointManager::PostUpdate()
{
	// Debug rendering (only if physics debug mode is active)
	if (App && App->physics && App->physics->IsDebugMode())
	{
		DebugRender();
	}

	return UPDATE_CONTINUE;
}

bool CheckpointManager::CleanUp()
{
	LOG("Cleaning up Checkpoint Manager");

	// Sensors are destroyed by ModulePhysics automatically
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

	// Iterate through all map objects
	int checkpointCount = 0;
	for (const auto& object : App->map->mapData.objects)
	{
		LOG("Object found: name='%s', type='%s', at (%d,%d) size (%d,%d)",
			object->name.c_str(),
			object->type.c_str(),
			object->x, object->y,
			object->width, object->height);

		// List all properties of this object
		LOG("  Properties count: %d", (int)object->properties.propertyList.size());
		for (const auto& prop : object->properties.propertyList)
		{
			LOG("    Property: %s = %s", prop->name.c_str(), prop->value.c_str());
		}

		// Look for "Order" property to identify checkpoints
		Properties::Property* orderProp = object->properties.GetProperty("Order");

		if (!orderProp)
		{
			// Not a checkpoint - skip it
			LOG("  -> Skipped (no Order property)");
			continue;
		}

		// This object HAS the Order property - it's a checkpoint!
		checkpointCount++;
		LOG("  -> CHECKPOINT DETECTED (has Order property)!");

		// Parse order value
		int order = std::stoi(orderProp->value);
		LOG("  -> Order value: %d", order);

		// Create checkpoint sensor
		CreateCheckpointSensor(object, order);

		LOG("Loaded checkpoint: %s (Order: %d) at (%d, %d)",
			object->name.c_str(), order, object->x, object->y);
	}

	LOG("Total checkpoints processed: %d", checkpointCount);

	// Find finish line for quick reference
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

	// LOG DETALLADO: Coordenadas originales de Tiled
	LOG("=== CREATING SENSOR FOR %s ===", object->name.c_str());
	LOG("  Tiled coords (raw): X=%d Y=%d W=%d H=%d", object->x, object->y, object->width, object->height);

	// CRITICAL FIX: The "Checkpoints" layer in Tiled has an offset!
	// offsetx="32" offsety="240" (from Map.tmx)
	// We need to ADD this offset to the object's position
	const float CHECKPOINTS_LAYER_OFFSET_X = 32.0f;
	const float CHECKPOINTS_LAYER_OFFSET_Y = 240.0f;

	// Apply layer offset to get world coordinates
	float worldX = (float)object->x + CHECKPOINTS_LAYER_OFFSET_X;
	float worldY = (float)object->y + CHECKPOINTS_LAYER_OFFSET_Y;

	LOG("  With layer offset: X=%.2f Y=%.2f", worldX, worldY);

	// Calculate center position
	float centerX = worldX + (float)object->width * 0.5f;
	float centerY = worldY + (float)object->height * 0.5f;

	LOG("  Calculated center (pixels): (%.2f, %.2f)", centerX, centerY);

	// Create static rectangle sensor (EXACTAS dimensiones de Tiled)
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

	// Configure as sensor (no physical collision, just detection)
	sensor->SetSensor(true);

	// VERIFICACIÓN: Leer la posición que Box2D realmente almacenó
	float verifyX, verifyY;
	sensor->GetPositionF(verifyX, verifyY);
	LOG("  Box2D returned position (pixels): (%.2f, %.2f)", verifyX, verifyY);
	LOG("  Difference: deltaX=%.2f deltaY=%.2f", verifyX - centerX, verifyY - centerY);

	// Create checkpoint data
	Checkpoint checkpoint;
	checkpoint.order = order;
	checkpoint.name = object->name;
	checkpoint.sensor = sensor;
	checkpoint.crossed = false;

	// Add to list BEFORE setting user data
	checkpoints.push_back(checkpoint);

	// CRITICAL FIX: Get stable pointer AFTER push_back
	Checkpoint* stablePtr = &checkpoints.back();

	// Store reference to THIS CHECKPOINT (not the manager) in sensor user data
	stablePtr->sensor->SetUserData(stablePtr);

	// Register THIS MANAGER as collision listener for this sensor
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
	// Check if collision is with player car
	if (!other || other != playerBody)
		return;

	LOG("Player collision with checkpoint sensor detected!");

	// Get player position for logging
	float carX, carY;
	other->GetPositionF(carX, carY);

	// Box2D already confirmed collision - find the closest checkpoint
	// This will be the one we actually collided with
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

	// Process the checkpoint we hit
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

	// Find the checkpoint by order
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

		// Can only cross finish line if all checkpoints are crossed
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
			// Valid lap completion!
			currentLap++;
			LOG("=== LAP %d COMPLETE! ===", currentLap - 1);
			LOG("Starting lap %d", currentLap);

			// Reset all checkpoints for next lap
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

	// Check if this is the next expected checkpoint
	if (checkpoint->order == nextCheckpointOrder)
	{
		if (!checkpoint->crossed)
		{
			checkpoint->crossed = true;
			LOG("=== Checkpoint %s crossed! (%d/%d) ===",
				checkpoint->name.c_str(), checkpoint->order, totalCheckpoints);

			// Move to next checkpoint
			nextCheckpointOrder++;

			// If all checkpoints crossed, next is finish line
			if (nextCheckpointOrder > totalCheckpoints)
			{
				nextCheckpointOrder = 0;  // 0 = finish line
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
	// Check if all checkpoints are crossed and ready for finish line
	for (const auto& cp : checkpoints)
	{
		if (cp.order > 0 && !cp.crossed)
			return false;
	}
	return true;
}

void CheckpointManager::DebugRender() const
{
	// Draw checkpoint information overlay
	int yOffset = 180;  // Below physics debug info

	DrawRectangle(10, yOffset, 300, 160, Fade(BLACK, 0.8f));
	DrawRectangleLines(10, yOffset, 300, 160, GREEN);

	DrawText("=== CHECKPOINTS ===", 20, yOffset + 10, 20, GREEN);
	DrawText(TextFormat("Lap: %d", currentLap), 20, yOffset + 35, 18, WHITE);

	// Show next checkpoint name
	const char* nextName = "FL";
	if (nextCheckpointOrder > 0 && nextCheckpointOrder <= 5)
	{
		nextName = TextFormat("C%d", nextCheckpointOrder);
	}
	DrawText(TextFormat("Next: %s", nextName), 20, yOffset + 55, 18, YELLOW);

	// Show checkpoint status
	int crossed = 0;
	for (const auto& cp : checkpoints)
	{
		if (cp.order > 0 && cp.crossed)
			crossed++;
	}
	DrawText(TextFormat("Progress: %d/%d", crossed, totalCheckpoints), 20, yOffset + 75, 18,
		crossed == totalCheckpoints ? GREEN : WHITE);

	// Show if ready for finish line
	if (IsLapComplete())
	{
		DrawText(">>> READY FOR FINISH! <<<", 20, yOffset + 95, 16, GREEN);
	}

	// Legend
	DrawText("Purple = Sensors (hitboxes)", 20, yOffset + 115, 14, PURPLE);
	DrawText("Cross = Center point", 20, yOffset + 135, 14, WHITE);

	// Draw checkpoint names in world space (within BeginMode2D)
	for (const auto& checkpoint : checkpoints)
	{
		if (!checkpoint.sensor)
			continue;

		float x, y;
		checkpoint.sensor->GetPositionF(x, y);

		// Color based on status
		Color textColor = WHITE;
		if (checkpoint.crossed)
			textColor = GREEN;
		else if (checkpoint.order == nextCheckpointOrder)
			textColor = YELLOW;

		// Measure text to center it properly
		const char* nameText = checkpoint.name.c_str();
		int fontSize = 20;
		int textWidth = MeasureText(nameText, fontSize);

		// Draw checkpoint name CENTERED above the sensor
		DrawText(nameText, (int)x - textWidth / 2, (int)y - 50, fontSize, textColor);

		// Draw order number CENTERED below the center
		const char* orderText = TextFormat("(%d)", checkpoint.order);
		int orderWidth = MeasureText(orderText, 16);
		DrawText(orderText, (int)x - orderWidth / 2, (int)y + 30, 16, textColor);

		// Draw a LARGE CROSS at the exact center for verification
		DrawLine((int)x - 20, (int)y, (int)x + 20, (int)y, textColor);
		DrawLine((int)x, (int)y - 20, (int)x, (int)y + 20, textColor);

		// Draw a small circle at center
		DrawCircle((int)x, (int)y, 3, textColor);
	}
}