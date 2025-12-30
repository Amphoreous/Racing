#pragma once

#include "core/Module.h"
#include "core/Globals.h"
#include "raylib.h"

class PhysBody;
class Car;

// PushAbility: Special ability that creates a circular area that pushes away enemy cars
class PushAbility
{
public:
	PushAbility();
	~PushAbility();

	// Initialize ability
	bool Init(Application* app);

	// Activate the ability at the player's position with player's rotation
	void Activate(float playerX, float playerY, float playerRotation);

	// Update ability (handles animation, duration, effects)
	void Update();

	// Draw the ability effect
	void Draw() const;

	// Check if ability is ready to use
	bool IsReady() const;

	// Check if ability is currently active
	bool IsActive() const;

	// Get cooldown progress (0.0 to 1.0)
	float GetCooldownProgress() const;

private:
	Application* app;

	// Ability state
	bool active;
	float activeTimer;          // Time ability has been active
	float activeDuration;       // How long ability lasts (seconds)

	float cooldownTimer;        // Time since last use
	float cooldownDuration;     // Cooldown between uses (seconds)
	bool wasCooldownReady;      // Track if cooldown was ready last frame (for sound trigger)

	// Effect properties
	float centerX, centerY;     // Center of the push area
	float playerRotation;       // Player's rotation when ability was activated
	float pushRadius;           // Radius of effect (pixels)
	float pushForce;            // Force applied to enemies (pixels/second)

	// Visual effect
	Texture2D effectTexture;    // space_effect.png
	float effectScale;          // Current scale of effect
	float effectRotation;       // Current rotation for animation (relative to player rotation)
	float maxScale;             // Maximum scale when fully expanded

	// Push sensor (physics body)
	PhysBody* pushSensor;       // Circular sensor to detect NPCs

	// Sound effects
	unsigned int abilitySfxId;           // ability.wav
	unsigned int cooldownReadySfxId;     // cd_ability_down.wav

	// Helper methods
	void CreatePushSensor();
	void DestroyPushSensor();
	void ApplyPushToNearbyNPCs();
};