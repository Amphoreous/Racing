#include "PhysBody.h"
#include "Globals.h"
#include "box2d/box2d.h"

// Physics constants
#define METERS_TO_PIXELS 50.0f
#define PIXELS_TO_METERS (1.0f / METERS_TO_PIXELS)
#define DEGREES_TO_RADIANS (b2_pi / 180.0f)
#define RADIANS_TO_DEGREES (180.0f / b2_pi)

PhysBody::PhysBody() : body(nullptr), userData(nullptr), collisionListener(nullptr)
{
}

PhysBody::~PhysBody()
{
	// Body is destroyed by Box2D world, not here
	// We just clean up our wrapper
}

// Position and rotation getters/setters
void PhysBody::GetPosition(int& x, int& y) const
{
	if (!body) return;
	b2Vec2 pos = body->GetPosition();
	x = static_cast<int>(pos.x * METERS_TO_PIXELS);
	y = static_cast<int>(pos.y * METERS_TO_PIXELS);
}

void PhysBody::GetPositionF(float& x, float& y) const
{
	if (!body) return;
	b2Vec2 pos = body->GetPosition();
	x = pos.x * METERS_TO_PIXELS;
	y = pos.y * METERS_TO_PIXELS;
}

void PhysBody::SetPosition(float x, float y)
{
	if (!body) return;
	body->SetTransform(b2Vec2(x * PIXELS_TO_METERS, y * PIXELS_TO_METERS), body->GetAngle());
}

float PhysBody::GetRotation() const
{
	if (!body) return 0.0f;
	return body->GetAngle() * RADIANS_TO_DEGREES;
}

void PhysBody::SetRotation(float degrees)
{
	if (!body) return;
	body->SetTransform(body->GetPosition(), degrees * DEGREES_TO_RADIANS);
}

// === VELOCITY ===
void PhysBody::GetLinearVelocity(float& vx, float& vy) const
{
	if (!body) return;
	b2Vec2 vel = body->GetLinearVelocity();
	vx = vel.x * METERS_TO_PIXELS;
	vy = vel.y * METERS_TO_PIXELS;
}

void PhysBody::SetLinearVelocity(float vx, float vy)
{
	if (!body) return;
	body->SetLinearVelocity(b2Vec2(vx * PIXELS_TO_METERS, vy * PIXELS_TO_METERS));
}

float PhysBody::GetAngularVelocity() const
{
	if (!body) return 0.0f;
	return body->GetAngularVelocity() * RADIANS_TO_DEGREES;
}

void PhysBody::SetAngularVelocity(float omega)
{
	if (!body) return;
	body->SetAngularVelocity(omega * DEGREES_TO_RADIANS);
}

// === FORCES AND IMPULSES ===
void PhysBody::ApplyForce(float fx, float fy)
{
	if (!body) return;
	body->ApplyForceToCenter(b2Vec2(fx, fy), true);
}

void PhysBody::ApplyForceAtPoint(float fx, float fy, float pointX, float pointY)
{
	if (!body) return;
	b2Vec2 force(fx, fy);
	b2Vec2 point(pointX * PIXELS_TO_METERS, pointY * PIXELS_TO_METERS);
	body->ApplyForce(force, point, true);
}

void PhysBody::ApplyLinearImpulse(float ix, float iy)
{
	if (!body) return;
	body->ApplyLinearImpulseToCenter(b2Vec2(ix, iy), true);
}

void PhysBody::ApplyLinearImpulseAtPoint(float ix, float iy, float pointX, float pointY)
{
	if (!body) return;
	b2Vec2 impulse(ix, iy);
	b2Vec2 point(pointX * PIXELS_TO_METERS, pointY * PIXELS_TO_METERS);
	body->ApplyLinearImpulse(impulse, point, true);
}

void PhysBody::ApplyTorque(float torque)
{
	if (!body) return;
	body->ApplyTorque(torque, true);
}

void PhysBody::ApplyAngularImpulse(float impulse)
{
	if (!body) return;
	body->ApplyAngularImpulse(impulse, true);
}

// === BODY PROPERTIES ===
void PhysBody::SetBodyType(BodyType type)
{
	if (!body) return;
	
	b2BodyType b2Type;
	switch (type)
	{
	case BodyType::STATIC:
		b2Type = b2_staticBody;
		break;
	case BodyType::KINEMATIC:
		b2Type = b2_kinematicBody;
		break;
	case BodyType::DYNAMIC:
		b2Type = b2_dynamicBody;
		break;
	default:
		b2Type = b2_dynamicBody;
		break;
	}
	
	body->SetType(b2Type);
}

PhysBody::BodyType PhysBody::GetBodyType() const
{
	if (!body) return BodyType::STATIC;
	
	switch (body->GetType())
	{
	case b2_staticBody:
		return BodyType::STATIC;
	case b2_kinematicBody:
		return BodyType::KINEMATIC;
	case b2_dynamicBody:
		return BodyType::DYNAMIC;
	default:
		return BodyType::STATIC;
	}
}

void PhysBody::SetActive(bool active)
{
	if (!body) return;
	body->SetEnabled(active);
}

bool PhysBody::IsActive() const
{
	if (!body) return false;
	return body->IsEnabled();
}

void PhysBody::SetFixedRotation(bool fixed)
{
	if (!body) return;
	body->SetFixedRotation(fixed);
}

bool PhysBody::IsFixedRotation() const
{
	if (!body) return false;
	return body->IsFixedRotation();
}

void PhysBody::SetGravityScale(float scale)
{
	if (!body) return;
	body->SetGravityScale(scale);
}

float PhysBody::GetGravityScale() const
{
	if (!body) return 1.0f;
	return body->GetGravityScale();
}

// === PHYSICAL PROPERTIES ===
b2Fixture* PhysBody::GetMainFixture() const
{
	if (!body) return nullptr;
	return body->GetFixtureList();
}

void PhysBody::SetDensity(float density)
{
	b2Fixture* fixture = GetMainFixture();
	if (!fixture) return;
	
	fixture->SetDensity(density);
	body->ResetMassData();
}

void PhysBody::SetFriction(float friction)
{
	b2Fixture* fixture = GetMainFixture();
	if (!fixture) return;
	fixture->SetFriction(friction);
}

void PhysBody::SetRestitution(float restitution)
{
	b2Fixture* fixture = GetMainFixture();
	if (!fixture) return;
	fixture->SetRestitution(restitution);
}

float PhysBody::GetMass() const
{
	if (!body) return 0.0f;
	return body->GetMass();
}

float PhysBody::GetInertia() const
{
	if (!body) return 0.0f;
	return body->GetInertia();
}

// === COLLISION PROPERTIES ===
void PhysBody::SetSensor(bool isSensor)
{
	b2Fixture* fixture = GetMainFixture();
	if (!fixture) return;
	fixture->SetSensor(isSensor);
}

bool PhysBody::IsSensor() const
{
	b2Fixture* fixture = GetMainFixture();
	if (!fixture) return false;
	return fixture->IsSensor();
}

void PhysBody::SetCategoryBits(unsigned short category)
{
	b2Fixture* fixture = GetMainFixture();
	if (!fixture) return;
	
	b2Filter filter = fixture->GetFilterData();
	filter.categoryBits = category;
	fixture->SetFilterData(filter);
}

void PhysBody::SetMaskBits(unsigned short mask)
{
	b2Fixture* fixture = GetMainFixture();
	if (!fixture) return;
	
	b2Filter filter = fixture->GetFilterData();
	filter.maskBits = mask;
	fixture->SetFilterData(filter);
}

void PhysBody::SetGroupIndex(short group)
{
	b2Fixture* fixture = GetMainFixture();
	if (!fixture) return;
	
	b2Filter filter = fixture->GetFilterData();
	filter.groupIndex = group;
	fixture->SetFilterData(filter);
}

// === USER DATA ===
void PhysBody::SetUserData(void* data)
{
	userData = data;
}

void* PhysBody::GetUserData() const
{
	return userData;
}

// === COLLISION CALLBACKS ===
void PhysBody::SetCollisionListener(CollisionListener* listener)
{
	collisionListener = listener;
}

PhysBody::CollisionListener* PhysBody::GetCollisionListener() const
{
	return collisionListener;
}
