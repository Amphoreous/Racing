#include "entities/PushAbility.h"
#include "core/Application.h"
#include "modules/ModulePhysics.h"
#include "modules/ModuleResources.h"
#include "modules/ModuleAudio.h"
#include "entities/NPCManager.h"
#include "entities/PhysBody.h"
#include "entities/Car.h"
#include "core/p2Point.h"
#include <cmath>

// Ability configuration
#define PUSH_RADIUS 150.0f
#define PUSH_FORCE 2000.0f
#define ACTIVE_DURATION 0.5f
#define COOLDOWN_DURATION 5.0f
#define EFFECT_MAX_SCALE 0.5f
#define EFFECT_ROTATION_SPEED 360.0f

PushAbility::PushAbility()
	: app(nullptr)
	, active(false)
	, activeTimer(0.0f)
	, activeDuration(ACTIVE_DURATION)
	, cooldownTimer(0.0f)
	, cooldownDuration(COOLDOWN_DURATION)
	, wasCooldownReady(false)
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
	, abilitySfxId(0)
	, cooldownReadySfxId(0)
{
}

PushAbility::~PushAbility()
{
	DestroyPushSensor();
}

bool PushAbility::Init(Application* application)
{
	app = application;

	effectTexture = app->resources->LoadTexture("assets/sprites/space_effect.png");
	if (effectTexture.id == 0)
	{
		LOG("WARNING: Failed to load push ability effect texture");
		return false;
	}

	if (app->audio)
	{
		abilitySfxId = app->audio->LoadFx("assets/audio/fx/ability.wav");
		cooldownReadySfxId = app->audio->LoadFx("assets/audio/fx/cd_ability_down.wav");
		LOG("Push ability sound effects loaded (Ability: %u, Cooldown: %u)", abilitySfxId, cooldownReadySfxId);
	}

	LOG("Push ability initialized successfully (Texture size: %dx%d)", effectTexture.width, effectTexture.height);
	return true;
}

void PushAbility::Activate(float playerX, float playerY, float rotation)
{
	if (!IsReady())
	{
		LOG("Push ability on cooldown! %.1fs remaining", cooldownDuration - cooldownTimer);
		return;
	}

	LOG("=== PUSH ABILITY ACTIVATED ===");
	LOG("Position: (%.1f, %.1f), Rotation: %.1f°", playerX, playerY, rotation);

	// Play ability.wav
	if (app && app->audio && abilitySfxId > 0)
	{
		app->audio->PlayFx(abilitySfxId);
	}

	active = true;
	activeTimer = 0.0f;
	cooldownTimer = 0.0f;
	wasCooldownReady = false;

	centerX = playerX;
	centerY = playerY;
	playerRotation = rotation;

	effectScale = 0.0f;
	effectRotation = 0.0f;

	CreatePushSensor();
	ApplyPushToNearbyNPCs();
}

void PushAbility::Update()
{
	float deltaTime = GetFrameTime();

	// Update cooldown
	if (cooldownTimer < cooldownDuration)
	{
		cooldownTimer += deltaTime;

		// Check if cooldown just finished
		if (cooldownTimer >= cooldownDuration && !wasCooldownReady)
		{
			// Play cd_ability_down.wav
			if (app && app->audio && cooldownReadySfxId > 0)
			{
				app->audio->PlayFx(cooldownReadySfxId);
				LOG("Ability cooldown ready!");
			}
			wasCooldownReady = true;
		}
	}

	// Update active ability
	if (active)
	{
		activeTimer += deltaTime;

		float expandProgress = activeTimer / activeDuration;
		effectScale = expandProgress * maxScale;

		effectRotation += EFFECT_ROTATION_SPEED * deltaTime;
		if (effectRotation >= 360.0f)
			effectRotation -= 360.0f;

		ApplyPushToNearbyNPCs();

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

	float currentWidth = effectTexture.width * effectScale;
	float currentHeight = effectTexture.height * effectScale;

	float alpha = 1.0f - (activeTimer / activeDuration);
	alpha = alpha * alpha;

	float totalRotation = playerRotation + effectRotation;

	Rectangle source = { 0, 0, (float)effectTexture.width, (float)effectTexture.height };
	Rectangle dest = { centerX, centerY, currentWidth, currentHeight };
	Vector2 origin = { currentWidth * 0.5f, currentHeight * 0.5f };

	Color tint = { 255, 255, 255, (unsigned char)(alpha * 255) };

	DrawTexturePro(effectTexture, source, dest, origin, totalRotation, tint);

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
		return 1.0f;

	return cooldownTimer / cooldownDuration;
}

void PushAbility::CreatePushSensor()
{
	if (!app || !app->physics)
		return;

	pushSensor = app->physics->CreateCircle(centerX, centerY, pushRadius, PhysBody::BodyType::STATIC);
	if (pushSensor)
	{
		pushSensor->SetSensor(true);
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

	float forceMult = 1.0f - (activeTimer / activeDuration);
	forceMult = forceMult * forceMult;

	const std::vector<Car*>& npcs = app->npcManager->GetNPCs();

	int pushedCount = 0;
	for (Car* npc : npcs)
	{
		if (!npc || !npc->GetPhysBody())
			continue;

		float npcX, npcY;
		npc->GetPosition(npcX, npcY);

		float dx = npcX - centerX;
		float dy = npcY - centerY;
		float distance = sqrtf(dx * dx + dy * dy);

		if (distance < pushRadius && distance > 0.1f)
		{
			float dirX = dx / distance;
			float dirY = dy / distance;

			float distanceRatio = 1.0f - (distance / pushRadius);
			float currentForce = pushForce * distanceRatio * forceMult;

			float forceX = dirX * currentForce;
			float forceY = dirY * currentForce;

			npc->GetPhysBody()->ApplyForce(forceX, forceY);

			pushedCount++;

			if (app->physics->IsDebugMode())
			{
				DrawLine((int)centerX, (int)centerY, (int)npcX, (int)npcY, RED);
			}
		}
	}

	if (pushedCount > 0 && activeTimer < 0.1f)
	{
		LOG("Pushed %d NPCs", pushedCount);
	}
}