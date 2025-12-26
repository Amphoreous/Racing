#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModulePhysics.h"
#include "PhysBody.h"
#include "Player.h"
#include "Car.h"
#include "Entity.h"
#include "Player.h"

#include "box2d/box2d.h"
#include "box2d/b2_mouse_joint.h"
#include "raylib.h"
#include <math.h>

// Physics constants
#define METERS_TO_PIXELS 50.0f
#define PIXELS_TO_METERS (1.0f / METERS_TO_PIXELS)
#define GRAVITY_X 0.0f
#define GRAVITY_Y 10.0f  // 10 m/s^2 downward
#define VELOCITY_ITERATIONS 8
#define POSITION_ITERATIONS 3

// Contact listener for collision callbacks
class ModulePhysics::PhysicsContactListener : public b2ContactListener
{
public:
	void BeginContact(b2Contact* contact) override
	{
		PhysBody* bodyA = (PhysBody*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
		PhysBody* bodyB = (PhysBody*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;
		
		if (bodyA && bodyA->GetCollisionListener())
			bodyA->GetCollisionListener()->OnCollisionEnter(bodyB);
		
		if (bodyB && bodyB->GetCollisionListener())
			bodyB->GetCollisionListener()->OnCollisionEnter(bodyA);
	}
	
	void EndContact(b2Contact* contact) override
	{
		PhysBody* bodyA = (PhysBody*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
		PhysBody* bodyB = (PhysBody*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;
		
		if (bodyA && bodyA->GetCollisionListener())
			bodyA->GetCollisionListener()->OnCollisionExit(bodyB);
		
		if (bodyB && bodyB->GetCollisionListener())
			bodyB->GetCollisionListener()->OnCollisionExit(bodyA);
	}
};

ModulePhysics::ModulePhysics(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	world = nullptr;
	contactListener = nullptr;
	debugMode = true;
	mouseJoint = nullptr;
	draggedBody = nullptr;
	groundBody = nullptr;
}

ModulePhysics::~ModulePhysics()
{
}

bool ModulePhysics::Start()
{
	LOG("Creating Physics 2D environment");
	
	// Create Box2D world with gravity
	b2Vec2 gravity(GRAVITY_X, GRAVITY_Y);
	world = new b2World(gravity);
	
	if (!world)
	{
		LOG("ERROR: Failed to create Box2D world");
		return false;
	}
	
	// Set up contact listener for collision callbacks
	contactListener = new PhysicsContactListener();
	world->SetContactListener(contactListener);
	
	// Create ground body for mouse joint
	b2BodyDef groundBodyDef;
	groundBodyDef.type = b2_staticBody;
	groundBodyDef.position.Set(0.0f, 0.0f);
	groundBody = world->CreateBody(&groundBodyDef);
	
	LOG("Physics world created successfully");
	return true;
}

update_status ModulePhysics::PreUpdate()
{
	if (!world)
		return UPDATE_CONTINUE;
	
	// Step the physics simulation
	// Note: Using fixed timestep (1/60th of a second)
	float timeStep = 1.0f / 60.0f;
	world->Step(timeStep, VELOCITY_ITERATIONS, POSITION_ITERATIONS);
	
	return UPDATE_CONTINUE;
}

// 
update_status ModulePhysics::PostUpdate()
{
	// Toggle debug mode with F1
	if (IsKeyPressed(KEY_F1))
	{
		debugMode = !debugMode;
		LOG("Physics debug mode: %s", debugMode ? "ON" : "OFF");
	}

	// Handle mouse joint for dragging bodies in debug mode
	if (debugMode)
	{
		HandleMouseJoint();
		DebugDraw();
	}

	return UPDATE_CONTINUE;
}

bool ModulePhysics::CleanUp()
{
	LOG("Destroying physics world");
	
	// Destroy all physics bodies
	for (PhysBody* body : bodies)
	{
		if (body)
		{
			delete body;
		}
	}
	bodies.clear();
	
	// Delete contact listener
	if (contactListener)
	{
		delete contactListener;
		contactListener = nullptr;
	}
	
	// Destroy mouse joint if active
	if (mouseJoint)
	{
		if (world)
		{
			world->DestroyJoint(mouseJoint);
		}
		mouseJoint = nullptr;
		draggedBody = nullptr;
	}
	
	// Delete Box2D world (this destroys all b2Body objects)
	if (world)
	{
		delete world;
		world = nullptr;
	}
	
	LOG("Physics world destroyed");
	return true;
}

// Body creation methods
PhysBody* ModulePhysics::CreateCircle(float x, float y, float radius, PhysBody::BodyType bodyType)
{
	if (!world)
	{
		LOG("ERROR: Cannot create circle - world not initialized");
		return nullptr;
	}
	
	// Create Box2D body definition
	b2BodyDef bodyDef;
	bodyDef.position.Set(x * PIXELS_TO_METERS, y * PIXELS_TO_METERS);
	
	// Set body type
	switch (bodyType)
	{
	case PhysBody::BodyType::STATIC:
		bodyDef.type = b2_staticBody;
		break;
	case PhysBody::BodyType::KINEMATIC:
		bodyDef.type = b2_kinematicBody;
		break;
	case PhysBody::BodyType::DYNAMIC:
		bodyDef.type = b2_dynamicBody;
		break;
	}
	
	// Create the body in Box2D
	b2Body* b2body = world->CreateBody(&bodyDef);
	
	// Create circle shape
	b2CircleShape shape;
	shape.m_radius = radius * PIXELS_TO_METERS;
	
	// Create fixture
	b2FixtureDef fixtureDef;
	fixtureDef.shape = &shape;
	fixtureDef.density = 1.0f;
	fixtureDef.friction = 0.3f;
	fixtureDef.restitution = 0.5f;
	
	b2body->CreateFixture(&fixtureDef);
	
	// Create PhysBody wrapper
	PhysBody* physBody = new PhysBody();
	physBody->SetB2Body(b2body);
	
	// Store wrapper pointer in Box2D body for collision callbacks
	b2body->GetUserData().pointer = (uintptr_t)physBody;
	
	// Add to our list
	bodies.push_back(physBody);
	
	LOG("Created circle body at (%.1f, %.1f) with radius %.1f", x, y, radius);
	return physBody;
}

PhysBody* ModulePhysics::CreateRectangle(float x, float y, float width, float height, PhysBody::BodyType bodyType)
{
	if (!world)
	{
		LOG("ERROR: Cannot create rectangle - world not initialized");
		return nullptr;
	}
	
	// Create Box2D body definition
	b2BodyDef bodyDef;
	bodyDef.position.Set(x * PIXELS_TO_METERS, y * PIXELS_TO_METERS);
	
	// Set body type
	switch (bodyType)
	{
	case PhysBody::BodyType::STATIC:
		bodyDef.type = b2_staticBody;
		break;
	case PhysBody::BodyType::KINEMATIC:
		bodyDef.type = b2_kinematicBody;
		break;
	case PhysBody::BodyType::DYNAMIC:
		bodyDef.type = b2_dynamicBody;
		break;
	}
	
	// Create the body in Box2D
	b2Body* b2body = world->CreateBody(&bodyDef);
	
	// Create box shape (Box2D uses half-widths)
	b2PolygonShape shape;
	shape.SetAsBox((width * 0.5f) * PIXELS_TO_METERS, (height * 0.5f) * PIXELS_TO_METERS);
	
	// Create fixture
	b2FixtureDef fixtureDef;
	fixtureDef.shape = &shape;
	fixtureDef.density = 1.0f;
	fixtureDef.friction = 0.3f;
	fixtureDef.restitution = 0.3f;
	
	b2body->CreateFixture(&fixtureDef);
	
	// Create PhysBody wrapper
	PhysBody* physBody = new PhysBody();
	physBody->SetB2Body(b2body);
	
	// Store wrapper pointer in Box2D body
	b2body->GetUserData().pointer = (uintptr_t)physBody;
	
	// Add to our list
	bodies.push_back(physBody);
	
	LOG("Created rectangle body at (%.1f, %.1f) with size %.1fx%.1f", x, y, width, height);
	return physBody;
}

PhysBody* ModulePhysics::CreatePolygon(float x, float y, const float* vertices, int vertexCount, PhysBody::BodyType bodyType)
{
	if (!world || !vertices || vertexCount < 3 || vertexCount > b2_maxPolygonVertices)
	{
		LOG("ERROR: Invalid polygon parameters");
		return nullptr;
	}
	
	// Create Box2D body definition
	b2BodyDef bodyDef;
	bodyDef.position.Set(x * PIXELS_TO_METERS, y * PIXELS_TO_METERS);
	
	// Set body type
	switch (bodyType)
	{
	case PhysBody::BodyType::STATIC:
		bodyDef.type = b2_staticBody;
		break;
	case PhysBody::BodyType::KINEMATIC:
		bodyDef.type = b2_kinematicBody;
		break;
	case PhysBody::BodyType::DYNAMIC:
		bodyDef.type = b2_dynamicBody;
		break;
	}
	
	// Create the body in Box2D
	b2Body* b2body = world->CreateBody(&bodyDef);
	
	// Convert vertices to Box2D format
	b2Vec2* b2vertices = new b2Vec2[vertexCount];
	for (int i = 0; i < vertexCount; i++)
	{
		b2vertices[i].Set(vertices[i * 2] * PIXELS_TO_METERS, vertices[i * 2 + 1] * PIXELS_TO_METERS);
	}
	
	// Create polygon shape
	b2PolygonShape shape;
	shape.Set(b2vertices, vertexCount);
	
	delete[] b2vertices;
	
	// Create fixture
	b2FixtureDef fixtureDef;
	fixtureDef.shape = &shape;
	fixtureDef.density = 1.0f;
	fixtureDef.friction = 0.3f;
	fixtureDef.restitution = 0.3f;
	
	b2body->CreateFixture(&fixtureDef);
	
	// Create PhysBody wrapper
	PhysBody* physBody = new PhysBody();
	physBody->SetB2Body(b2body);
	
	// Store wrapper pointer in Box2D body
	b2body->GetUserData().pointer = (uintptr_t)physBody;
	
	// Add to our list
	bodies.push_back(physBody);
	
	LOG("Created polygon body at (%.1f, %.1f) with %d vertices", x, y, vertexCount);
	return physBody;
}

PhysBody* ModulePhysics::CreateChain(float x, float y, const float* vertices, int vertexCount, bool loop)
{
	if (!world || !vertices || vertexCount < 2)
	{
		LOG("ERROR: Invalid chain parameters");
		return nullptr;
	}
	
	// Create Box2D body definition (chains are always static)
	b2BodyDef bodyDef;
	bodyDef.type = b2_staticBody;
	bodyDef.position.Set(x * PIXELS_TO_METERS, y * PIXELS_TO_METERS);
	
	// Create the body in Box2D
	b2Body* b2body = world->CreateBody(&bodyDef);
	
	// Convert vertices to Box2D format
	b2Vec2* b2vertices = new b2Vec2[vertexCount];
	for (int i = 0; i < vertexCount; i++)
	{
		b2vertices[i].Set(vertices[i * 2] * PIXELS_TO_METERS, vertices[i * 2 + 1] * PIXELS_TO_METERS);
	}
	
	// Create chain shape
	b2ChainShape shape;
	if (loop)
	{
		shape.CreateLoop(b2vertices, vertexCount);
	}
	else
	{
		shape.CreateChain(b2vertices, vertexCount, b2Vec2(0, 0), b2Vec2(0, 0));
	}
	
	delete[] b2vertices;
	
	// Create fixture
	b2FixtureDef fixtureDef;
	fixtureDef.shape = &shape;
	fixtureDef.friction = 0.5f;
	fixtureDef.restitution = 0.0f;
	
	b2body->CreateFixture(&fixtureDef);
	
	// Create PhysBody wrapper
	PhysBody* physBody = new PhysBody();
	physBody->SetB2Body(b2body);
	
	// Store wrapper pointer in Box2D body
	b2body->GetUserData().pointer = (uintptr_t)physBody;
	
	// Add to our list
	bodies.push_back(physBody);
	
	LOG("Created chain body at (%.1f, %.1f) with %d vertices (loop: %s)", x, y, vertexCount, loop ? "yes" : "no");
	return physBody;
}

void ModulePhysics::DestroyBody(PhysBody* body)
{
	if (!world || !body)
		return;
	
	b2Body* b2body = body->GetB2Body();
	if (b2body)
	{
		world->DestroyBody(b2body);
	}
	
	// Remove from our list
	for (auto it = bodies.begin(); it != bodies.end(); ++it)
	{
		if (*it == body)
		{
			bodies.erase(it);
			break;
		}
	}
	
	delete body;
	LOG("Destroyed physics body");
}

// World properties
void ModulePhysics::SetGravity(float gx, float gy)
{
	if (!world) return;
	world->SetGravity(b2Vec2(gx, gy));
	LOG("Gravity set to (%.2f, %.2f)", gx, gy);
}

void ModulePhysics::GetGravity(float& gx, float& gy) const
{
	if (!world)
	{
		gx = gy = 0.0f;
		return;
	}
	
	b2Vec2 gravity = world->GetGravity();
	gx = gravity.x;
	gy = gravity.y;
}

void ModulePhysics::SetDebugMode(bool enabled)
{
	debugMode = enabled;
}

bool ModulePhysics::IsDebugMode() const
{
	return debugMode;
}

// Raycasting
class RaycastCallback : public b2RayCastCallback
{
public:
	RaycastCallback() : hit(false), body(nullptr), fraction(1.0f) {}
	
	float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override
	{
		this->hit = true;
		this->point = point;
		this->normal = normal;
		this->fraction = fraction;
		this->body = (PhysBody*)fixture->GetBody()->GetUserData().pointer;
		return fraction;
	}
	
	bool hit;
	PhysBody* body;
	b2Vec2 point;
	b2Vec2 normal;
	float fraction;
};

bool ModulePhysics::Raycast(float x1, float y1, float x2, float y2, PhysBody*& hitBody, float& hitX, float& hitY, float& hitNormalX, float& hitNormalY)
{
	if (!world) return false;
	
	b2Vec2 start(x1 * PIXELS_TO_METERS, y1 * PIXELS_TO_METERS);
	b2Vec2 end(x2 * PIXELS_TO_METERS, y2 * PIXELS_TO_METERS);
	
	RaycastCallback callback;
	world->RayCast(&callback, start, end);
	
	if (callback.hit)
	{
		hitBody = callback.body;
		hitX = callback.point.x * METERS_TO_PIXELS;
		hitY = callback.point.y * METERS_TO_PIXELS;
		hitNormalX = callback.normal.x;
		hitNormalY = callback.normal.y;
		return true;
	}
	
	hitBody = nullptr;
	return false;
}

class QueryCallback : public b2QueryCallback
{
public:
	QueryCallback(std::vector<PhysBody*>& bodies) : bodies(bodies) {}
	
	bool ReportFixture(b2Fixture* fixture) override
	{
		PhysBody* body = (PhysBody*)fixture->GetBody()->GetUserData().pointer;
		if (body)
		{
			bodies.push_back(body);
		}
		return true; // Continue query
	}
	
	std::vector<PhysBody*>& bodies;
};

int ModulePhysics::QueryArea(float minX, float minY, float maxX, float maxY, std::vector<PhysBody*>& outBodies)
{
	if (!world) return 0;
	
	b2AABB aabb;
	aabb.lowerBound.Set(minX * PIXELS_TO_METERS, minY * PIXELS_TO_METERS);
	aabb.upperBound.Set(maxX * PIXELS_TO_METERS, maxY * PIXELS_TO_METERS);
	
	QueryCallback callback(outBodies);
	world->QueryAABB(&callback, aabb);
	
	return (int)outBodies.size();
}

// Debug rendering
void ModulePhysics::DebugDraw()
{
#ifndef NDEBUG // Only include debug draw in debug builds
       if (!world) return;

       // --- Debug Info Overlay ---

	   int overlayX = 10, overlayY = 10, overlayW = 370, overlayH = 160;
	   // Use solid black background for readability
	   DrawRectangle(overlayX, overlayY, overlayW, overlayH, BLACK);
	   DrawRectangleLines(overlayX, overlayY, overlayW, overlayH, YELLOW);

	   // FPS
	   int fps = GetFPS();
	   DrawText(TextFormat("FPS: %d", fps), overlayX + 10, overlayY + 10, 22, WHITE);

	   // Body count
	   int bodyCount = 0;
	   for (b2Body* b = world->GetBodyList(); b; b = b->GetNext()) ++bodyCount;
	   DrawText(TextFormat("Bodies: %d", bodyCount), overlayX + 120, overlayY + 10, 22, WHITE);

	   // Mouse position
	   Vector2 mouse = GetMousePosition();
	   DrawText(TextFormat("Mouse: (%.0f, %.0f)", mouse.x, mouse.y), overlayX + 10, overlayY + 40, 20, WHITE);

	   // Gravity info
	   b2Vec2 gravity = world->GetGravity();
	   DrawText(TextFormat("Gravity: (%.2f, %.2f)", gravity.x, gravity.y), overlayX + 10, overlayY + 65, 20, WHITE);

	   // Step info
	   DrawText(TextFormat("Step: dt=1/60, VelIters=%d, PosIters=%d", VELOCITY_ITERATIONS, POSITION_ITERATIONS), overlayX + 10, overlayY + 90, 18, WHITE);

	   // World size
	   DrawText(TextFormat("World: %dx%d px", SCREEN_WIDTH, SCREEN_HEIGHT), overlayX + 10, overlayY + 110, 18, WHITE);

	   // Player vehicle position (if available)
	   float carX = 0, carY = 0;
	   bool hasCar = false;
	   if (App && App->player && App->player->GetCar()) {
		   App->player->GetCar()->GetPosition(carX, carY);
		   hasCar = true;
	   }
	   if (hasCar) {
		   DrawText(TextFormat("Car Pos: (%.1f, %.1f)", carX, carY), overlayX + 10, overlayY + 130, 20, YELLOW);
	   } else {
		   DrawText("Car Pos: (N/A)", overlayX + 10, overlayY + 130, 20, GRAY);
	   }

       // --- Draw all bodies ---
       for (b2Body* b = world->GetBodyList(); b; b = b->GetNext())
       {
	       // Color code by body type
	       Color color = GRAY;
	       switch (b->GetType()) {
		       case b2_staticBody: color = BLUE; break;
		       case b2_kinematicBody: color = ORANGE; break;
		       case b2_dynamicBody: color = GREEN; break;
		       default: color = GRAY; break;
	       }
	       for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext())
	       {
		       switch (f->GetType())
		       {
			       // Draw circles
			       case b2Shape::e_circle:
			       {
				       b2CircleShape* shape = (b2CircleShape*)f->GetShape();
				       b2Vec2 pos = b->GetPosition();
				       float px = pos.x * METERS_TO_PIXELS;
				       float py = pos.y * METERS_TO_PIXELS;
				       float radius = shape->m_radius * METERS_TO_PIXELS;
				       DrawCircleLines((int)px, (int)py, radius, color);
				       // Draw position cross
				       DrawLine((int)px-5, (int)py, (int)px+5, (int)py, YELLOW);
				       DrawLine((int)px, (int)py-5, (int)px, (int)py+5, YELLOW);
			       }
			       break;

			       // Draw polygons
			       case b2Shape::e_polygon:
			       {
				       b2PolygonShape* polygonShape = (b2PolygonShape*)f->GetShape();
				       int32 count = polygonShape->m_count;
				       b2Vec2 prev, v;
				       for (int32 i = 0; i < count; ++i)
				       {
					       v = b->GetWorldPoint(polygonShape->m_vertices[i]);
					       if (i > 0)
					       {
						       float x1 = prev.x * METERS_TO_PIXELS;
						       float y1 = prev.y * METERS_TO_PIXELS;
						       float x2 = v.x * METERS_TO_PIXELS;
						       float y2 = v.y * METERS_TO_PIXELS;
						       DrawLine((int)x1, (int)y1, (int)x2, (int)y2, color);
					       }
					       prev = v;
				       }
				       v = b->GetWorldPoint(polygonShape->m_vertices[0]);
				       float x1 = prev.x * METERS_TO_PIXELS;
				       float y1 = prev.y * METERS_TO_PIXELS;
				       float x2 = v.x * METERS_TO_PIXELS;
				       float y2 = v.y * METERS_TO_PIXELS;
				       DrawLine((int)x1, (int)y1, (int)x2, (int)y2, color);
			       }
			       break;

			       // Draw chains
			       case b2Shape::e_chain:
			       {
				       b2ChainShape* shape = (b2ChainShape*)f->GetShape();
				       b2Vec2 prev, v;
				       for (int32 i = 0; i < shape->m_count; ++i)
				       {
					       v = b->GetWorldPoint(shape->m_vertices[i]);
					       if (i > 0)
					       {
						       float x1 = prev.x * METERS_TO_PIXELS;
						       float y1 = prev.y * METERS_TO_PIXELS;
						       float x2 = v.x * METERS_TO_PIXELS;
						       float y2 = v.y * METERS_TO_PIXELS;
						       DrawLine((int)x1, (int)y1, (int)x2, (int)y2, color);
					       }
					       prev = v;
				       }
			       }
			       break;

			       // Draw edge shapes
			       case b2Shape::e_edge:
			       {
				       b2EdgeShape* shape = (b2EdgeShape*)f->GetShape();
				       b2Vec2 v1 = b->GetWorldPoint(shape->m_vertex1);
				       b2Vec2 v2 = b->GetWorldPoint(shape->m_vertex2);
				       float x1 = v1.x * METERS_TO_PIXELS;
				       float y1 = v1.y * METERS_TO_PIXELS;
				       float x2 = v2.x * METERS_TO_PIXELS;
				       float y2 = v2.y * METERS_TO_PIXELS;
				       DrawLine((int)x1, (int)y1, (int)x2, (int)y2, color);
			       }
			       break;
		       }
	       }
       }
#endif // NDEBUG
}

// Handle mouse joint for dragging bodies in debug mode
void ModulePhysics::HandleMouseJoint()
{
	if (!world) return;

	Vector2 mousePos = GetMousePosition();

	// Start dragging on left mouse press
	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !mouseJoint)
	{
		LOG("Mouse button pressed at (%.0f, %.0f)", mousePos.x, mousePos.y);
		LOG("Mouse button pressed at (%.0f, %.0f)", mousePos.x, mousePos.y);
		// Find closest body to mouse position
		PhysBody* closest = nullptr;
		float minDist = FLT_MAX;

		for (PhysBody* body : bodies)
		{
			if (!body || !body->IsActive()) continue;

			float x, y;
			body->GetPositionF(x, y);
			float dx = mousePos.x - x;
			float dy = mousePos.y - y;
			float dist = sqrtf(dx * dx + dy * dy);

			if (dist < minDist && dist < 100.0f) // Increased radius for selection
			{
				minDist = dist;
				closest = body;
			}
		}

		// Create mouse joint if body found
		if (closest)
		{
			draggedBody = closest;
			LOG("Mouse joint created for body at distance %.2f", minDist);

			b2MouseJointDef def;
			def.bodyA = groundBody; // Attach to ground
			def.bodyB = closest->GetB2Body();   // The body to drag
			def.target = b2Vec2(mousePos.x * PIXELS_TO_METERS, mousePos.y * PIXELS_TO_METERS);
			def.maxForce = 10000.0f * closest->GetMass(); // Stronger force to move body
			def.stiffness = 100.0f; // Linear stiffness in N/m
			def.damping = 10.0f; // Linear damping in N*s/m

			mouseJoint = (b2MouseJoint*)world->CreateJoint(&def);
		}
		else
		{
			LOG("No body found near mouse position (%.0f, %.0f)", mousePos.x, mousePos.y);
		}
	}

	// Update joint target while dragging
	if (mouseJoint && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
	{
		mouseJoint->SetTarget(b2Vec2(mousePos.x * PIXELS_TO_METERS, mousePos.y * PIXELS_TO_METERS));

		// Draw joint line (optional)
		if (draggedBody)
		{
			float x, y;
			draggedBody->GetPositionF(x, y);
			DrawLine((int)x, (int)y, (int)mousePos.x, (int)mousePos.y, RED);
		}
	}

	// Release joint on mouse release
	if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && mouseJoint)
	{
		world->DestroyJoint(mouseJoint);
		mouseJoint = nullptr;
		draggedBody = nullptr;
	}
}