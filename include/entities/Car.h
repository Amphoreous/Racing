#pragma once

#include "entities/Entity.h"
#include "raylib.h"
#include "core/p2Point.h"
#include <vector>

class Car : public Entity
{
public:
	// Terrain types that affect car physics
	enum TerrainType
	{
		NORMAL = 0,
		MUD,
		WATER
	};

	Car(Application* app);
	virtual ~Car();

	// Lifecycle
	bool Start() override;
	update_status Update() override;
	void Draw() const override;

	// Car controls
	void Accelerate(float amount);
	void Reverse(float amount);
	void Brake(float amount);
	void Steer(float direction);
	void Drift();

	// Car properties
	void SetMaxSpeed(float speed);
	float GetMaxSpeed() const { return maxSpeed; }
	float GetCurrentSpeed() const;

	void SetReverseSpeed(float speed);
	float GetReverseSpeed() const { return reverseMaxSpeed; }

	void SetAcceleration(float accel);
	float GetAcceleration() const { return accelerationForce; }

	void SetSteeringSensitivity(float sensitivity);
	float GetSteeringSensitivity() const { return steeringSensitivity; }

	// Visual
	void SetTexture(Texture2D tex);
	void SetColor(Color color);

	// Terrain detection
	TerrainType GetCurrentTerrain() const;
	void UpdateTerrainEffects();

private:
	// Physics tuning parameters
	float accelerationForce;
	float reverseForce;
	float brakeForce;
	float maxSpeed;
	float reverseMaxSpeed;
	float steeringSensitivity;
	float driftImpulse;

	// Rendering
	Texture2D texture;
	Color tint;
	float renderScale;

	// Terrain state
	TerrainType currentTerrain;
	float terrainFrictionModifier;
	float terrainAccelerationModifier;
	float terrainSpeedModifier;

	// Motor sound
	Music motorSound;
	bool isMotorPlaying;

	// Helper methods
	void ApplyFriction();
	void ClampSpeed();
	void ApplyDownforce();
	void UpdateMotorSound();
	vec2f GetForwardVector() const;
	vec2f GetRightVector() const;

	// Terrain detection
	bool IsPointInPolygon(float px, float py, const std::vector<vec2i>& points, float offsetX, float offsetY) const;
};