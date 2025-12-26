#pragma once

#include "entities/Entity.h"
#include "raylib.h"
#include "core/p2Point.h"

class Car : public Entity
{
public:
	Car(Application* app);
	virtual ~Car();

	// Lifecycle
	bool Start() override;
	update_status Update() override;
	void Draw() const override;

	// Car controls
	void Accelerate(float amount);      // Apply forward force (0.0 to 1.0)
	void Reverse(float amount);         // Apply reverse force (0.0 to 1.0)
	void Brake(float amount);           // Apply braking force (0.0 to 1.0)
	void Steer(float direction);        // Steer left (-1.0) or right (1.0)
	void Drift();                       // Apply lateral impulse for drifting

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

private:
	// Physics tuning parameters
	float accelerationForce;      // Forward force magnitude
	float reverseForce;           // Reverse force magnitude
	float brakeForce;             // Braking force magnitude
	float maxSpeed;               // Maximum forward speed clamp (pixels/sec)
	float reverseMaxSpeed;        // Maximum reverse speed clamp (pixels/sec)
	float steeringSensitivity;    // Angular velocity per steer input
	float driftImpulse;           // Lateral impulse for drifting

	// Rendering
	Texture2D texture;
	Color tint;
	float renderScale;

	// Helper methods
	void ApplyFriction();
	void ClampSpeed();
	void ApplyDownforce();
	vec2f GetForwardVector() const;
	vec2f GetRightVector() const;
};