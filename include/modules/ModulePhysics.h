#pragma once

#include "core/Module.h"
#include "core/Globals.h"
#include "entities/PhysBody.h"
#include <vector>

// Forward declarations - NEVER expose Box2D types in this header
class b2World;
class b2Body;
class b2ContactListener;

// Structure to store collision information for debug visualization
struct CollisionInfo
{
	float x, y;           // Collision point in world coordinates (pixels)
	float normalX, normalY; // Collision normal
	float separation;     // Penetration depth
};

// ModulePhysics: Complete wrapper for Box2D physics engine
// This module encapsulates ALL Box2D functionality
// Game code should NEVER include box2d.h or use b2* types directly
// All physics operations must go through this module's interface
class ModulePhysics : public Module
{
public:
	ModulePhysics(Application* app, bool start_enabled = true);
	~ModulePhysics();

	bool Init();
	bool Start();
	update_status PreUpdate();
	update_status PostUpdate();
	bool CleanUp();

	// Body creation functions
	// Create a circular physics body
	// Parameters:
	//   x, y: position in pixels
	//   radius: radius in pixels
	//   bodyType: STATIC (doesn't move), KINEMATIC (moves but not affected by forces), DYNAMIC (full physics)
	// Returns: PhysBody wrapper (never returns nullptr, check IsActive() if needed)
	PhysBody* CreateCircle(float x, float y, float radius, PhysBody::BodyType bodyType = PhysBody::BodyType::DYNAMIC);
	
	// Create a rectangular physics body
	// Parameters:
	//   x, y: center position in pixels
	//   width, height: dimensions in pixels
	//   bodyType: see CreateCircle
	PhysBody* CreateRectangle(float x, float y, float width, float height, PhysBody::BodyType bodyType = PhysBody::BodyType::DYNAMIC);
	
	// Create a polygon physics body
	// Parameters:
	//   x, y: center position in pixels
	//   vertices: array of x,y coordinates in pixels (relative to center)
	//   vertexCount: number of vertices
	//   bodyType: see CreateCircle
	// Note: vertices must form a convex polygon and be in counter-clockwise order
	PhysBody* CreatePolygon(float x, float y, const float* vertices, int vertexCount, PhysBody::BodyType bodyType = PhysBody::BodyType::DYNAMIC);
	
	// Create a chain (line strip) physics body - typically used for ground/walls
	// Parameters:
	//   x, y: offset position in pixels
	//   vertices: array of x,y coordinates in pixels
	//   vertexCount: number of vertices
	//   loop: if true, connects last vertex to first vertex
	// Note: Chains are always STATIC
	PhysBody* CreateChain(float x, float y, const float* vertices, int vertexCount, bool loop = false);
	
	// Destroy a physics body
	// WARNING: After calling this, the PhysBody pointer is invalid - set it to nullptr!
	void DestroyBody(PhysBody* body);

	// World properties
	// Set world gravity (pixels/second^2)
	void SetGravity(float gx, float gy);
	
	// Get world gravity
	void GetGravity(float& gx, float& gy) const;
	
	// Enable/disable debug rendering
	void SetDebugMode(bool enabled);
	bool IsDebugMode() const;

	// Raycasting
	// Cast a ray and get the first hit
	// Parameters:
	//   x1, y1: start point in pixels
	//   x2, y2: end point in pixels
	//   hitBody: [out] the body that was hit (or nullptr)
	//   hitX, hitY: [out] hit point in pixels
	//   hitNormalX, hitNormalY: [out] surface normal at hit point
	// Returns: true if something was hit
	bool Raycast(float x1, float y1, float x2, float y2, PhysBody*& hitBody, float& hitX, float& hitY, float& hitNormalX, float& hitNormalY);
	
	// Query bodies in a rectangular area
	// Parameters:
	//   minX, minY, maxX, maxY: rectangle bounds in pixels
	//   outBodies: [out] vector filled with bodies found
	// Returns: number of bodies found
	int QueryArea(float minX, float minY, float maxX, float maxY, std::vector<PhysBody*>& outBodies);

	// Joints (advanced stuff)
	// Create a distance joint (rope/spring connection between two bodies)
	// Create a revolute joint (hinge/pin connection)
	// Create a prismatic joint (slider connection)

	// Public method to render debug overlay
	void RenderDebug();

	// Public method to render physics debug visualization
	void DebugDraw();
	// Get number of active collisions for debug display
	int GetActiveCollisionCount() const { return (int)activeCollisions.size(); }
private:
	// Box2D world
	b2World* world;
	
	// Debug mode flag
	bool debugMode;
	
	// All physics bodies created by this module
	std::vector<PhysBody*> bodies;
	
	// Active collisions for debug visualization
	std::vector<CollisionInfo> activeCollisions;
	
	// Mouse joint for debug dragging
	class b2MouseJoint* mouseJoint;
	PhysBody* draggedBody;
	b2Body* groundBody;
	
	// Helper functions
	void HandleMouseJoint();
	
	// Contact listener for collision callbacks
	class PhysicsContactListener;
	PhysicsContactListener* contactListener;
};