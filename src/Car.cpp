#include "Car.h"
#include "Application.h"
#include "ModulePhysics.h"
#include "ModuleRender.h"
#include "PhysBody.h"
#include <math.h>

// Default car physics values (tunable)
#define DEFAULT_ACCELERATION 20.0f
#define DEFAULT_REVERSE_FORCE 10.0f
#define DEFAULT_BRAKE_FORCE 300.0f
#define DEFAULT_MAX_SPEED 100.0f
#define DEFAULT_REVERSE_MAX_SPEED 50.0f
#define DEFAULT_STEERING_SENSITIVITY 200.0f
#define DEFAULT_DRIFT_IMPULSE 500.0f
#define DEFAULT_CAR_WIDTH 40.0f
#define DEFAULT_CAR_HEIGHT 70.0f
#define FRICTION_COEFFICIENT 0.98f

Car::Car(Application* app)
	: Entity(app)
	, accelerationForce(DEFAULT_ACCELERATION)
	, reverseForce(DEFAULT_REVERSE_FORCE)
	, brakeForce(DEFAULT_BRAKE_FORCE)
	, maxSpeed(DEFAULT_MAX_SPEED)
	, reverseMaxSpeed(DEFAULT_REVERSE_MAX_SPEED)
	, steeringSensitivity(DEFAULT_STEERING_SENSITIVITY)
	, driftImpulse(DEFAULT_DRIFT_IMPULSE)
	, texture({ 0 })
	, tint(WHITE)
	, renderScale(1.0f)
{
}

Car::~Car()
{
	// Clean up physics body through ModulePhysics
	if (physBody && app && app->physics)
	{
		app->physics->DestroyBody(physBody);
		physBody = nullptr;
	}
}

bool Car::Start()
{
	LOG("Creating Car physics body");

	// Create rectangular physics body for the car
	physBody = app->physics->CreateRectangle(
		400.0f, 300.0f,  // Initial position (center of screen)
		DEFAULT_CAR_WIDTH, DEFAULT_CAR_HEIGHT,
		PhysBody::BodyType::DYNAMIC
	);

	if (!physBody)
	{
		LOG("ERROR: Failed to create Car physics body");
		return false;
	}

	// Configure physics properties
	physBody->SetDensity(1.0f);
	physBody->SetFriction(0.5f);
	physBody->SetRestitution(0.2f);
	physBody->SetLinearVelocity(0.0f, 0.0f);
	physBody->SetAngularVelocity(0.0f);

	// Disable gravity for top-down racing game
	physBody->SetGravityScale(0.0f);

	// Set user data to reference this car
	physBody->SetUserData(this);

	LOG("Car created successfully");
	return true;
}

update_status Car::Update()
{
	if (!active || !physBody)
		return UPDATE_CONTINUE;

	// Apply natural friction to simulate drag
	ApplyFriction();

	// Clamp speed to max speed
	ClampSpeed();

	return UPDATE_CONTINUE;
}

void Car::Draw() const
{
	if (!active || !physBody)
		return;

	float x, y;
	GetPosition(x, y);
	float rotation = GetRotation();

	// Draw car (placeholder rectangle if no texture)
	if (texture.id != 0)
	{
		// Calculate texture center
		Rectangle source = { 0, 0, (float)texture.width, (float)texture.height };
		Rectangle dest = { x, y, texture.width * renderScale, texture.height * renderScale };
		Vector2 origin = { (texture.width * renderScale) * 0.5f, (texture.height * renderScale) * 0.5f };

		DrawTexturePro(texture, source, dest, origin, rotation, tint);
	}
	else
	{
		// Fallback: draw colored rectangle
		DrawRectanglePro(
			{ x, y, DEFAULT_CAR_WIDTH, DEFAULT_CAR_HEIGHT },
			{ DEFAULT_CAR_WIDTH * 0.5f, DEFAULT_CAR_HEIGHT * 0.5f },
			rotation,
			tint
		);
	}
}

void Car::Accelerate(float amount)
{
	if (!physBody || amount <= 0.0f)
		return;

	// Clamp amount between 0 and 1
	amount = (amount > 1.0f) ? 1.0f : amount;

	// Get forward direction based on current rotation
	vec2f forward = GetForwardVector();

	// Apply force in forward direction
	float forceX = forward.x * accelerationForce * amount;
	float forceY = forward.y * accelerationForce * amount;

	physBody->ApplyForce(forceX, forceY);
}

void Car::Reverse(float amount)
{
	if (!physBody || amount <= 0.0f)
		return;

	// Clamp amount between 0 and 1
	amount = (amount > 1.0f) ? 1.0f : amount;

	// Get forward direction and reverse it
	vec2f forward = GetForwardVector();

	// Apply force in backward direction (negative forward)
	float forceX = -forward.x * reverseForce * amount;
	float forceY = -forward.y * reverseForce * amount;

	physBody->ApplyForce(forceX, forceY);
}

void Car::Brake(float amount)
{
	if (!physBody || amount <= 0.0f)
		return;

	// Clamp amount between 0 and 1
	amount = (amount > 1.0f) ? 1.0f : amount;

	// Get current velocity
	float vx, vy;
	physBody->GetLinearVelocity(vx, vy);

	// Apply force opposite to velocity direction
	float speed = sqrtf(vx * vx + vy * vy);
	if (speed > 0.1f)
	{
		float normalizedVx = vx / speed;
		float normalizedVy = vy / speed;

		float brakeX = -normalizedVx * brakeForce * amount;
		float brakeY = -normalizedVy * brakeForce * amount;

		physBody->ApplyForce(brakeX, brakeY);
	}
}

void Car::Steer(float direction)
{
	if (!physBody)
		return;

	// Clamp direction between -1 and 1
	direction = (direction < -1.0f) ? -1.0f : (direction > 1.0f) ? 1.0f : direction;

	// Apply angular velocity for steering
	float angularVelocity = direction * steeringSensitivity;
	physBody->SetAngularVelocity(angularVelocity);
}

void Car::Drift()
{
	if (!physBody)
		return;

	// Get right vector (perpendicular to forward)
	vec2f right = GetRightVector();

	// Get current velocity
	float vx, vy;
	physBody->GetLinearVelocity(vx, vy);

	// Calculate lateral velocity (velocity along right vector)
	float lateralVelocity = vx * right.x + vy * right.y;

	// Apply impulse opposite to lateral velocity to simulate drift
	float impulseX = -lateralVelocity * right.x * driftImpulse * 0.01f;
	float impulseY = -lateralVelocity * right.y * driftImpulse * 0.01f;

	physBody->ApplyLinearImpulse(impulseX, impulseY);
}

void Car::SetMaxSpeed(float speed)
{
	maxSpeed = speed;
}

void Car::SetReverseSpeed(float speed)
{
	reverseMaxSpeed = speed;
}

float Car::GetCurrentSpeed() const
{
	if (!physBody)
		return 0.0f;

	float vx, vy;
	physBody->GetLinearVelocity(vx, vy);
	return sqrtf(vx * vx + vy * vy);
}

void Car::SetAcceleration(float accel)
{
	accelerationForce = accel;
}

void Car::SetSteeringSensitivity(float sensitivity)
{
	steeringSensitivity = sensitivity;
}

void Car::SetTexture(Texture2D tex)
{
	texture = tex;
}

void Car::SetColor(Color color)
{
	tint = color;
}

void Car::ApplyFriction()
{
	if (!physBody)
		return;

	// Get current velocity
	float vx, vy;
	physBody->GetLinearVelocity(vx, vy);

	// Apply friction coefficient
	physBody->SetLinearVelocity(vx * FRICTION_COEFFICIENT, vy * FRICTION_COEFFICIENT);
}

void Car::ClampSpeed()
{
	if (!physBody)
		return;

	float vx, vy;
	physBody->GetLinearVelocity(vx, vy);

	float speed = sqrtf(vx * vx + vy * vy);

	// Check if moving forward or backward relative to car's facing direction
	vec2f forward = GetForwardVector();
	float forwardDot = vx * forward.x + vy * forward.y;

	// Clamp based on direction
	float speedLimit = (forwardDot >= 0.0f) ? maxSpeed : reverseMaxSpeed;

	if (speed > speedLimit)
	{
		float scale = speedLimit / speed;
		physBody->SetLinearVelocity(vx * scale, vy * scale);
	}
}

vec2f Car::GetForwardVector() const
{
	if (!physBody)
		return vec2f(0.0f, -1.0f);

	// Convert rotation from degrees to radians
	float angleRad = GetRotation() * (3.14159265f / 180.0f);

	// Forward vector based on rotation (0 degrees = up in screen space)
	vec2f forward;
	forward.x = sinf(angleRad);
	forward.y = -cosf(angleRad);

	return forward;
}

vec2f Car::GetRightVector() const
{
	if (!physBody)
		return vec2f(1.0f, 0.0f);

	// Convert rotation from degrees to radians
	float angleRad = GetRotation() * (3.14159265f / 180.0f);

	// Right vector is perpendicular to forward
	vec2f right;
	right.x = cosf(angleRad);
	right.y = sinf(angleRad);

	return right;
}