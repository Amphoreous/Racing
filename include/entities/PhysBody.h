#pragma once

// Forward declarations - NEVER include box2d.h in this header
class b2Body;
class b2Fixture;

// PhysBody: Wrapper class for Box2D physics bodies
// This class provides a clean interface to Box2D physics without exposing Box2D types
// All interaction with physics should go through this class, NOT directly through Box2D
class PhysBody
{
public:
	PhysBody();
	~PhysBody();

	// Position and rotation
	// Get position in pixels (not meters)
	void GetPosition(int& x, int& y) const;
	void GetPositionF(float& x, float& y) const;
	
	// Set position in pixels (not meters)
	void SetPosition(float x, float y);
	
	// Get rotation in degrees
	float GetRotation() const;
	
	// Set rotation in degrees
	void SetRotation(float degrees);

	// Velocity stuff
	// Get linear velocity in pixels/second
	void GetLinearVelocity(float& vx, float& vy) const;
	
	// Set linear velocity in pixels/second
	void SetLinearVelocity(float vx, float vy);
	
	// Get angular velocity in degrees/second
	float GetAngularVelocity() const;
	
	// Set angular velocity in degrees/second
	void SetAngularVelocity(float omega);

	// Forces and impulses
	// Apply force at center of mass (in pixels)
	void ApplyForce(float fx, float fy);
	
	// Apply force at specific point (in pixels)
	void ApplyForceAtPoint(float fx, float fy, float pointX, float pointY);
	
	// Apply impulse at center of mass (instant velocity change)
	void ApplyLinearImpulse(float ix, float iy);
	
	// Apply impulse at specific point
	void ApplyLinearImpulseAtPoint(float ix, float iy, float pointX, float pointY);
	
	// Apply torque (rotational force)
	void ApplyTorque(float torque);
	
	// Apply angular impulse (instant angular velocity change)
	void ApplyAngularImpulse(float impulse);

	// Body properties
	// Get/set body type (static, kinematic, dynamic)
	enum class BodyType
	{
		STATIC,
		KINEMATIC,
		DYNAMIC
	};
	
	void SetBodyType(BodyType type);
	BodyType GetBodyType() const;
	
	// Enable/disable the body
	void SetActive(bool active);
	bool IsActive() const;
	
	// Enable/disable rotation
	void SetFixedRotation(bool fixed);
	bool IsFixedRotation() const;
	
	// Set gravity scale (1.0 = normal, 0.0 = no gravity)
	void SetGravityScale(float scale);
	float GetGravityScale() const;

	// Physical properties
	// Set density (affects mass)
	void SetDensity(float density);
	
	// Set friction (0.0 = ice, 1.0 = rough)
	void SetFriction(float friction);
	
	// Set restitution/bounciness (0.0 = no bounce, 1.0 = perfect bounce)
	void SetRestitution(float restitution);
	
	// Get mass in kilograms
	float GetMass() const;
	
	// Get inertia (rotational mass)
	float GetInertia() const;

	// Collision properties
	// Enable/disable collision detection
	void SetSensor(bool isSensor);
	bool IsSensor() const;
	
	// Check if this body is a real obstacle (static, non-sensor)
	bool IsStaticObstacle() const;
	
	// Set collision category (what I am)
	void SetCategoryBits(unsigned short category);
	
	// Set collision mask (what I collide with)
	void SetMaskBits(unsigned short mask);
	
	// Set group index (positive = always collide, negative = never collide)
	void SetGroupIndex(short group);

	// User data
	// Store custom game data pointer (e.g., pointer to game entity)
	void SetUserData(void* data);
	void* GetUserData() const;

	// Collision callbacks
	// Collision listener interface - implement this in your game objects
	class CollisionListener
	{
	public:
		virtual ~CollisionListener() {}
		virtual void OnCollisionEnter(PhysBody* other) {}
		virtual void OnCollisionExit(PhysBody* other) {}
		virtual void OnCollisionStay(PhysBody* other) {}
	};
	
	void SetCollisionListener(CollisionListener* listener);
	CollisionListener* GetCollisionListener() const;

private:
	// Allow ModulePhysics to access internal Box2D body
	friend class ModulePhysics;
	
	b2Body* GetB2Body() const { return body; }
	void SetB2Body(b2Body* b) { body = b; }
	
	b2Body* body;
	void* userData;
	CollisionListener* collisionListener;

	b2Fixture* GetMainFixture() const;
};
