#include "Entity.h"
#include "Application.h"

Entity::Entity(Application* app)
	: app(app), physBody(nullptr), active(true)
{
}

Entity::~Entity()
{
	// Derived classes should handle physics body destruction
	// through ModulePhysics->DestroyBody()
}

void Entity::GetPosition(float& x, float& y) const
{
	if (physBody)
	{
		physBody->GetPositionF(x, y);
	}
	else
	{
		x = y = 0.0f;
	}
}

void Entity::SetPosition(float x, float y)
{
	if (physBody)
	{
		physBody->SetPosition(x, y);
	}
}

float Entity::GetRotation() const
{
	return physBody ? physBody->GetRotation() : 0.0f;
}

void Entity::SetRotation(float degrees)
{
	if (physBody)
	{
		physBody->SetRotation(degrees);
	}
}

void Entity::SetActive(bool active)
{
	this->active = active;
	if (physBody)
	{
		physBody->SetActive(active);
	}
}

bool Entity::IsActive() const
{
	return active;
}