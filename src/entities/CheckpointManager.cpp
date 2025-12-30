#include "entities/CheckpointManager.h"
#include "core/Application.h"
#include "core/Map.h"
#include "modules/ModulePhysics.h"
#include "modules/ModuleAudio.h"
#include "modules/ModuleResources.h"
#include "modules/ModuleRender.h"
#include "entities/Player.h"
#include "entities/NPCManager.h"
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
	, countdownBeepSfxId(0)
	, countdownGoSfxId(0)
	, winBackground({0})
	, winBackgroundLoaded(false)
	, raceState(RACE_INTRO)
	, introTimer(0.0f)
	, countdownTimer(4.0f)
	, lastCountdownNumber(4)
	, introDuration(3.0f)
	, introStartX(0.0f)
	, introStartY(0.0f)
	, introEndX(0.0f)
	, introEndY(0.0f)
{
}

CheckpointManager::~CheckpointManager()
{
}

bool CheckpointManager::Start()
{
	LOG("Initializing Checkpoint Manager");

	// CRITICAL: Clean up existing data before re-initialization
	// This prevents memory leaks when Start() is called multiple times (Enable/Disable cycles)
	if (!checkpoints.empty())
	{
		LOG("Checkpoint data exists - cleaning up before re-initialization");
		checkpoints.clear();
		finishLine = nullptr;
	}
	
	// Reset race state
	currentLap = 1;
	nextCheckpointOrder = 1;
	raceFinished = false;
	
	// Initialize get ready, intro and countdown
	raceState = RACE_GET_READY;
	getReadyTimer = 0.0f;
	getReadyDuration = 3.0f;  // 3 second pause showing "GET READY!"
	introTimer = 0.0f;
	countdownTimer = 4.0f;
	lastCountdownNumber = 4;
	introDuration = 3.0f;

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

	// Load sound effects
	if (App->audio)
	{
		lapCompleteSfxId = App->audio->LoadFx("assets/audio/fx/checkpoint.wav");
		countdownBeepSfxId = App->audio->LoadFx("assets/audio/fx/checkpoint.wav"); // Reuse checkpoint sound for countdown beeps
	}
	
	// Set up intro camera path - pan from map overview to player position
	// Start at map center (overview) - convert tile coordinates to pixel coordinates
	if (App->map)
	{
		introStartX = (float)(App->map->mapData.width * App->map->mapData.tileWidth) * 0.5f;
		introStartY = (float)(App->map->mapData.height * App->map->mapData.tileHeight) * 0.5f;
	}
	else
	{
		introStartX = 2750.0f;
		introStartY = 1680.0f;
	}
	
	// End at player position and rotation
	if (App->player && App->player->GetCar())
	{
		App->player->GetCar()->GetPosition(introEndX, introEndY);
		introEndRotation = -App->player->GetCar()->GetRotation(); // Negated for camera
	}
	else
	{
		introEndX = 2000.0f;
		introEndY = 1400.0f;
		introEndRotation = 0.0f;
	}

	LOG("CheckpointManager initialized - %d checkpoints loaded", (int)checkpoints.size());
	LOG("Race configuration: %d laps, %d checkpoints per lap", totalLaps, totalCheckpoints);
	LOG("Current lap: %d, Next checkpoint: %d", currentLap, nextCheckpointOrder);
	LOG("Starting race intro - camera will pan from (%.0f, %.0f) to (%.0f, %.0f)", 
	    introStartX, introStartY, introEndX, introEndY);

	return true;
}

update_status CheckpointManager::Update()
{
	switch (raceState)
	{
	case RACE_GET_READY:
		UpdateGetReady();
		break;
	case RACE_INTRO:
		UpdateIntro();
		break;
	case RACE_COUNTDOWN:
		UpdateCountdown();
		break;
	case RACE_RUNNING:
		// Normal race logic handled elsewhere
		break;
	case RACE_FINISHED:
		// Win state - nothing to update
		break;
	}
	
	return UPDATE_CONTINUE;
}

void CheckpointManager::UpdateGetReady()
{
	// Clamp delta time to prevent skipping due to long loading frame
	float deltaTime = GetFrameTime();
	if (deltaTime > 0.1f) deltaTime = 0.1f; // Cap at 100ms per frame
	
	getReadyTimer += deltaTime;
	
	// Set camera to overview position (zoomed out, no rotation)
	if (App->renderer)
	{
		App->renderer->camera.target.x = introStartX;
		App->renderer->camera.target.y = introStartY;
		App->renderer->camera.rotation = 0.0f;
		App->renderer->camera.zoom = 0.15f;  // Zoomed out overview
	}
	
	// Transition to intro (camera pan) when GET READY pause is done
	if (getReadyTimer >= getReadyDuration)
	{
		raceState = RACE_INTRO;
		introTimer = 0.0f;
		LOG("GET READY complete - starting intro camera pan");
	}
}

void CheckpointManager::UpdateIntro()
{
	// Clamp delta time to prevent skipping intro due to long loading frame
	float deltaTime = GetFrameTime();
	if (deltaTime > 0.1f) deltaTime = 0.1f; // Cap at 100ms per frame
	
	introTimer += deltaTime;
	
	// Calculate interpolation factor (0 to 1)
	float t = introTimer / introDuration;
	if (t > 1.0f) t = 1.0f;
	
	// Smooth easing (ease-in-out)
	float smoothT = t * t * (3.0f - 2.0f * t);
	
	// Interpolate camera position
	float camX = introStartX + (introEndX - introStartX) * smoothT;
	float camY = introStartY + (introEndY - introStartY) * smoothT;
	
	// Update camera target directly
	if (App->renderer)
	{
		App->renderer->camera.target.x = camX;
		App->renderer->camera.target.y = camY;
		
		// Smooth rotation from 0 to player's starting rotation
		App->renderer->camera.rotation = introEndRotation * smoothT;
		
		// Zoom from overview to normal
		float startZoom = 0.15f;
		float endZoom = 1.0f;
		App->renderer->camera.zoom = startZoom + (endZoom - startZoom) * smoothT;
	}
	
	// Transition to countdown when intro is done
	if (introTimer >= introDuration)
	{
		raceState = RACE_COUNTDOWN;
		countdownTimer = 4.0f;
		lastCountdownNumber = 4;
		LOG("Intro complete - starting countdown");
	}
}

void CheckpointManager::UpdateCountdown()
{
	countdownTimer -= GetFrameTime();
	
	// Get current countdown number
	int currentNumber = (int)countdownTimer;
	
	// Play beep sound when number changes
	if (currentNumber != lastCountdownNumber && currentNumber >= 0)
	{
		lastCountdownNumber = currentNumber;
		if (App->audio && countdownBeepSfxId > 0 && currentNumber > 0)
		{
			App->audio->PlayFx(countdownBeepSfxId);
		}
	}
	
	// Transition to racing when countdown reaches 0
	if (countdownTimer <= 0.0f)
	{
		raceState = RACE_RUNNING;
		LOG("GO! Race started!");
		
		// Play a "GO" sound (reuse beep for now)
		if (App->audio && countdownBeepSfxId > 0)
		{
			App->audio->PlayFx(countdownBeepSfxId);
		}
	}
}

update_status CheckpointManager::PostUpdate()
{
	// Win screen is now drawn by ModuleRender::PostUpdate() in screen space
	// Just load the texture here if needed
	if (raceFinished && !winBackgroundLoaded)
	{
		winBackground = App->resources->LoadTexture("assets/ui/backgrounds/second_background.png");
		winBackgroundLoaded = true;
	}

	return UPDATE_CONTINUE;
}

void CheckpointManager::DrawWinScreen()
{
	// First draw a solid black rectangle to cover EVERYTHING
	DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);

	// Draw background fullscreen
	if (winBackgroundLoaded)
	{
		Rectangle source = {0, 0, (float)winBackground.width, (float)winBackground.height};
		Rectangle dest = {0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
		Vector2 origin = {0, 0};
		DrawTexturePro(winBackground, source, dest, origin, 0, WHITE);
	}

	// Semi-transparent overlay
	DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.4f));

	// Draw "YOU WIN!!" text
	const char* winText = "YOU WIN!!";
	int winFontSize = 80;
	int winWidth = MeasureText(winText, winFontSize);
	DrawText(winText,
		SCREEN_WIDTH / 2 - winWidth / 2,
		SCREEN_HEIGHT / 2 - 120,
		winFontSize,
		GOLD);

	// Draw position text
	const char* positionText = "Race Complete!";
	int posFontSize = 50;
	int posWidth = MeasureText(positionText, posFontSize);
	DrawText(positionText,
		SCREEN_WIDTH / 2 - posWidth / 2,
		SCREEN_HEIGHT / 2 - 20,
		posFontSize,
		WHITE);

	// Draw laps completed
	const char* lapsText = TextFormat("Completed %d laps!", totalLaps);
	int lapsFontSize = 30;
	int lapsWidth = MeasureText(lapsText, lapsFontSize);
	DrawText(lapsText,
		SCREEN_WIDTH / 2 - lapsWidth / 2,
		SCREEN_HEIGHT / 2 + 50,
		lapsFontSize,
		LIGHTGRAY);

}

void CheckpointManager::DrawCountdown()
{
	if (raceState == RACE_GET_READY || raceState == RACE_INTRO)
	{
		// Draw "GET READY" text during get ready and intro phases
		const char* readyText = "GET READY!";
		int fontSize = 60;
		int textWidth = MeasureText(readyText, fontSize);
		
		// Draw shadow
		DrawText(readyText,
			SCREEN_WIDTH / 2 - textWidth / 2 + 3,
			SCREEN_HEIGHT / 2 - 30 + 3,
			fontSize,
			BLACK);
		
		// Draw text
		DrawText(readyText,
			SCREEN_WIDTH / 2 - textWidth / 2,
			SCREEN_HEIGHT / 2 - 30,
			fontSize,
			YELLOW);
	}
	else if (raceState == RACE_COUNTDOWN)
	{
		int currentNumber = (int)countdownTimer;
		
		if (currentNumber > 0)
		{
			// Draw countdown number (3, 2, 1)
			const char* numText = TextFormat("%d", currentNumber);
			int fontSize = 200;
			int textWidth = MeasureText(numText, fontSize);
			
			// Pulsing effect based on fractional part of timer
			float pulse = 1.0f + 0.2f * (countdownTimer - (float)currentNumber);
			int displaySize = (int)(fontSize * pulse);
			int displayWidth = MeasureText(numText, displaySize);
			
			// Color based on number
			Color numColor;
			if (currentNumber == 3) numColor = RED;
			else if (currentNumber == 2) numColor = ORANGE;
			else numColor = YELLOW;
			
			// Draw shadow
			DrawText(numText,
				SCREEN_WIDTH / 2 - displayWidth / 2 + 5,
				SCREEN_HEIGHT / 2 - displaySize / 2 + 5,
				displaySize,
				BLACK);
			
			// Draw number
			DrawText(numText,
				SCREEN_WIDTH / 2 - displayWidth / 2,
				SCREEN_HEIGHT / 2 - displaySize / 2,
				displaySize,
				numColor);
		}
		else
		{
			// Draw "GO!" when countdown reaches 0
			const char* goText = "GO!";
			int fontSize = 200;
			int textWidth = MeasureText(goText, fontSize);
			
			// Draw shadow
			DrawText(goText,
				SCREEN_WIDTH / 2 - textWidth / 2 + 5,
				SCREEN_HEIGHT / 2 - fontSize / 2 + 5,
				fontSize,
				BLACK);
			
			// Draw GO!
			DrawText(goText,
				SCREEN_WIDTH / 2 - textWidth / 2,
				SCREEN_HEIGHT / 2 - fontSize / 2,
				fontSize,
				GREEN);
		}
	}
}

bool CheckpointManager::CleanUp()
{
	LOG("Cleaning up Checkpoint Manager");

	checkpoints.clear();
	finishLine = nullptr;
	playerBody = nullptr;

	// Unload win background if loaded
	if (winBackgroundLoaded)
	{
		App->resources->UnloadTexture("assets/ui/backgrounds/second_background.png");
		winBackgroundLoaded = false;
		winBackground = {0};
	}

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