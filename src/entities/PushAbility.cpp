#include "entities/PushAbility.h"
#include "core/Application.h"
#include "modules/ModulePhysics.h"
#include "modules/ModuleResources.h"
#include "entities/NPCManager.h"
#include "entities/PhysBody.h"
#include "entities/Car.h"
#include "core/p2Point.h"
#include <cmath>

// Ability configuration
#define PUSH_RADIUS 150.0f              // Effect radius in pixels
#define PUSH_FORCE 2000.0f              // Force magnitude
#define ACTIVE_DURATION 0.5f            // How long the push lasts (seconds)
#define COOLDOWN_DURATION 5.0f          // Cooldown between uses (seconds)
#define EFFECT_MAX_SCALE 0.5f           // Maximum scale of visual effect (adjusted size)
#define EFFECT_ROTATION_SPEED 360.0f    // Degrees per second (relative rotation)

PushAbility::PushAbility()
	: app(nullptr)
	, active(false)
	, activeTimer(0.0f)
	, activeDuration(ACTIVE_DURATION)
	, cooldownTimer(0.0f)
	, cooldownDuration(COOLDOWN_DURATION)
	, centerX(0.0f)
	, centerY(0.0f)
	, playerRotation(0.0f)
	, pushRadius(PUSH_RADIUS)
	, pushForce(PUSH_FORCE)
	, effectTexture({ 0 })
	, effectScale(0.0f)
	, effectRotation(0.0f)
	, maxScale(EFFECT_MAX_SCALE)
	, pushSensor(nullptr)
{
}

PushAbility::~PushAbility()
{
	DestroyPushSensor();
}

bool PushAbility::Init(Application* application)
{
	app = application;

	// Load effect texture
	effectTexture = app->resources->LoadTexture("assets/sprites/space_effect.png");
	if (effectTexture.id == 0)
	{
		LOG("WARNING: Failed to load push ability effect texture");
		return false;
	}

	LOG("Push ability initialized successfully (Texture size: %dx%d)", effectTexture.width, effectTexture.height);
	return true;
}

void PushAbility::Activate(float playerX, float playerY, float rotation)
{
	// Check if ability is ready
	if (!IsReady())
	{
		LOG("Push ability on cooldown! %.1fs remaining", cooldownDuration - cooldownTimer);
		return;
	}

	LOG("=== PUSH ABILITY ACTIVATED ===");
	LOG("Position: (%.1f, %.1f), Rotation: %.1f°", playerX, playerY, rotation);

	// Set ability as active
	active = true;
	activeTimer = 0.0f;
	cooldownTimer = 0.0f;

	// Store center position and rotation
	centerX = playerX;
	centerY = playerY;
	playerRotation = rotation;

	// Reset visual effect (relative rotation starts at 0)
	effectScale = 0.0f;
	effectRotation = 0.0f;

	// Create physics sensor for push area
	CreatePushSensor();

	// Immediately apply push force
	ApplyPushToNearbyNPCs();
}

void PushAbility::Update()
{
	float deltaTime = GetFrameTime();

	// Update cooldown timer
	if (cooldownTimer < cooldownDuration)
	{
		cooldownTimer += deltaTime;
	}

	// Update active ability
	if (active)
	{
		activeTimer += deltaTime;

		// Expand effect (grows quickly at start)
		float expandProgress = activeTimer / activeDuration;
		effectScale = expandProgress * maxScale;

		// Rotate effect for visual appeal (relative to player rotation)
		effectRotation += EFFECT_ROTATION_SPEED * deltaTime;
		if (effectRotation >= 360.0f)
			effectRotation -= 360.0f;

		// Continue applying push force (gradually weakens)
		ApplyPushToNearbyNPCs();

		// Check if ability duration ended
		if (activeTimer >= activeDuration)
		{
			LOG("Push ability ended");
			active = false;
			DestroyPushSensor();
		}
	}
}

void PushAbility::Draw() const
{
	if (!active || effectTexture.id == 0)
		return;

	// Calculate current size based on scale
	float currentWidth = effectTexture.width * effectScale;
	float currentHeight = effectTexture.height * effectScale;

	// Calculate alpha fade (starts strong, fades out)
	float alpha = 1.0f - (activeTimer / activeDuration);
	alpha = alpha * alpha;  // Squared for smoother fade

	// CRITICAL FIX: Use ONLY player rotation (no offset needed)
	// Just add the spinning animation to the player's rotation
	float totalRotation = playerRotation + effectRotation;

	// Draw the expanding, rotating effect
	Rectangle source = { 0, 0, (float)effectTexture.width, (float)effectTexture.height };
	Rectangle dest = { centerX, centerY, currentWidth, currentHeight };
	Vector2 origin = { currentWidth * 0.5f, currentHeight * 0.5f };

	Color tint = { 255, 255, 255, (unsigned char)(alpha * 255) };

	DrawTexturePro(effectTexture, source, dest, origin, totalRotation, tint);

	// Optional: Draw debug circle for push radius
	if (app && app->physics && app->physics->IsDebugMode())
	{
		DrawCircleLines((int)centerX, (int)centerY, pushRadius, ColorAlpha(YELLOW, 0.5f));
		DrawText("PUSH AREA", (int)centerX - 40, (int)centerY - 10, 20, YELLOW);
		DrawText(TextFormat("Rot: %.1f°", playerRotation), (int)centerX - 40, (int)centerY + 10, 16, YELLOW);
		DrawText(TextFormat("Total: %.1f°", totalRotation), (int)centerX - 40, (int)centerY + 30, 16, YELLOW);
		DrawText(TextFormat("Size: %.0fx%.0f", currentWidth, currentHeight), (int)centerX - 40, (int)centerY + 50, 16, YELLOW);
	}
}

bool PushAbility::IsReady() const
{
	return !active && (cooldownTimer >= cooldownDuration);
}

bool PushAbility::IsActive() const
{
	return active;
}

float PushAbility::GetCooldownProgress() const
{
	if (cooldownTimer >= cooldownDuration)
		return 1.0f;  // Ready

	return cooldownTimer / cooldownDuration;
}

void PushAbility::CreatePushSensor()
{
	if (!app || !app->physics)
		return;

	// Create circular sensor at push center
	pushSensor = app->physics->CreateCircle(centerX, centerY, pushRadius, PhysBody::BodyType::STATIC);
	if (pushSensor)
	{
		pushSensor->SetSensor(true);  // Don't physically collide, just detect
		LOG("Push sensor created at (%.1f, %.1f) with radius %.1f", centerX, centerY, pushRadius);
	}
}

void PushAbility::DestroyPushSensor()
{
	if (pushSensor && app && app->physics)
	{
		app->physics->DestroyBody(pushSensor);
		pushSensor = nullptr;
	}
}

void PushAbility::ApplyPushToNearbyNPCs()
{
	if (!app || !app->npcManager)
		return;

	// Calculate force multiplier (weakens over time)
	float forceMult = 1.0f - (activeTimer / activeDuration);
	forceMult = forceMult * forceMult;  // Squared for stronger initial push

	const std::vector<Car*>& npcs = app->npcManager->GetNPCs();

	int pushedCount = 0;
	for (Car* npc : npcs)
	{
		if (!npc || !npc->GetPhysBody())
			continue;

		// Get NPC position
		float npcX, npcY;
		npc->GetPosition(npcX, npcY);

		// Calculate distance from push center
		float dx = npcX - centerX;
		float dy = npcY - centerY;
		float distance = sqrtf(dx * dx + dy * dy);

		// Check if NPC is within push radius
		if (distance < pushRadius && distance > 0.1f)  // Avoid division by zero
		{
			// Normalize direction vector
			float dirX = dx / distance;
			float dirY = dy / distance;

			// Calculate push force (stronger closer to center)
			float distanceRatio = 1.0f - (distance / pushRadius);
			float currentForce = pushForce * distanceRatio * forceMult;

			// Apply radial push force (away from center)
			float forceX = dirX * currentForce;
			float forceY = dirY * currentForce;

			npc->GetPhysBody()->ApplyForce(forceX, forceY);

			pushedCount++;

			// Debug visualization
			if (app->physics->IsDebugMode())
			{
				DrawLine((int)centerX, (int)centerY, (int)npcX, (int)npcY, RED);
			}
		}
	}

	if (pushedCount > 0 && activeTimer < 0.1f)  // Log only at start
	{
		LOG("Pushed %d NPCs", pushedCount);
	}
}