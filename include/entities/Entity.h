#pragma once

#include "core/Globals.h"
#include "entities/PhysBody.h"
#include "raylib.h"

class Application;

class Entity
{
public:
	Entity(Application* app);
	virtual ~Entity();

	// Core lifecycle methods - to be overridden by derived classes
	virtual bool Start() { return true; }
	virtual update_status Update() { return UPDATE_CONTINUE; }
	virtual void Draw() const {}

	// Physics body access
	PhysBody* GetPhysBody() const { return physBody; }
	void SetPhysBody(PhysBody* body) { physBody = body; }

	// Position/rotation helpers (delegates to physics body)
	void GetPosition(float& x, float& y) const;
	void SetPosition(float x, float y);
	float GetRotation() const;
	void SetRotation(float degrees);

	// Active state
	void SetActive(bool active);
	bool IsActive() const;

protected:
	Application* app;
	PhysBody* physBody;
	bool active;
};