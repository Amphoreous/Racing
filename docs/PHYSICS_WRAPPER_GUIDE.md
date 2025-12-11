# Physics Wrapper System Documentation

## Overview
The Physics Wrapper System provides a complete encapsulation of Box2D physics engine. **CRITICAL**: Game code must NEVER include `box2d.h` or use any `b2*` types directly. All physics operations must go through `ModulePhysics` and `PhysBody`.

## Architecture

```
Game Code (ModuleGame, etc.)
    ↓
PhysBody (Wrapper)
    ↓
ModulePhysics (Wrapper)
    ↓
Box2D (Hidden Implementation)
```

## ModulePhysics API

### Body Creation

#### CreateCircle
```cpp
PhysBody* CreateCircle(float x, float y, float radius, PhysBody::BodyType bodyType)
```
Creates a circular physics body.
- **Parameters**:
  - `x, y`: Position in pixels (screen coordinates)
  - `radius`: Circle radius in pixels
  - `bodyType`: STATIC (unmovable), KINEMATIC (movable but not affected by forces), DYNAMIC (full physics simulation)
- **Returns**: PhysBody wrapper pointer
- **Example**:
```cpp
// Create a dynamic ball
PhysBody* ball = App->physics->CreateCircle(400, 300, 20, PhysBody::BodyType::DYNAMIC);
ball->SetRestitution(0.8f); // Make it bouncy
```

#### CreateRectangle
```cpp
PhysBody* CreateRectangle(float x, float y, float width, float height, PhysBody::BodyType bodyType)
```
Creates a rectangular physics body.
- **Parameters**:
  - `x, y`: Center position in pixels
  - `width, height`: Dimensions in pixels
  - `bodyType`: See CreateCircle
- **Example**:
```cpp
// Create a static platform
PhysBody* platform = App->physics->CreateRectangle(400, 500, 200, 20, PhysBody::BodyType::STATIC);
platform->SetFriction(0.5f);
```

#### CreatePolygon
```cpp
PhysBody* CreatePolygon(float x, float y, const float* vertices, int vertexCount, PhysBody::BodyType bodyType)
```
Creates a polygonal physics body.
- **Parameters**:
  - `x, y`: Center position in pixels
  - `vertices`: Array of x,y coordinates (relative to center) `[x1, y1, x2, y2, ...]`
  - `vertexCount`: Number of vertices (max 8 for Box2D)
  - `bodyType`: See CreateCircle
- **Note**: Vertices must form a convex polygon in counter-clockwise order
- **Example**:
```cpp
// Create a triangle
float triangleVertices[] = {
    0, -30,   // Top
    -20, 30,  // Bottom left
    20, 30    // Bottom right
};
PhysBody* triangle = App->physics->CreatePolygon(400, 300, triangleVertices, 3, PhysBody::BodyType::DYNAMIC);
```

#### CreateChain
```cpp
PhysBody* CreateChain(float x, float y, const float* vertices, int vertexCount, bool loop)
```
Creates a chain (line strip) physics body - typically used for ground, walls, or terrain.
- **Parameters**:
  - `x, y`: Offset position in pixels
  - `vertices`: Array of x,y coordinates `[x1, y1, x2, y2, ...]`
  - `vertexCount`: Number of vertices
  - `loop`: If true, connects last vertex back to first
- **Note**: Chains are always STATIC
- **Example**:
```cpp
// Create ground terrain
float groundVertices[] = {
    0, 0,
    100, 50,
    200, 50,
    300, 100,
    400, 100
};
PhysBody* ground = App->physics->CreateChain(0, 500, groundVertices, 5, false);
```

#### DestroyBody
```cpp
void DestroyBody(PhysBody* body)
```
Destroys a physics body and frees memory.
- **WARNING**: After calling this, set your pointer to nullptr!
- **Example**:
```cpp
App->physics->DestroyBody(myBody);
myBody = nullptr;  // IMPORTANT!
```

### World Properties

#### SetGravity / GetGravity
```cpp
void SetGravity(float gx, float gy)
void GetGravity(float& gx, float& gy)
```
Set/get world gravity in pixels per second squared.
- **Example**:
```cpp
// Set Earth-like gravity
App->physics->SetGravity(0.0f, 980.0f);  // 9.8 m/s² = 980 pixels/s²

// Set moon gravity (1/6th of Earth)
App->physics->SetGravity(0.0f, 163.3f);

// Zero gravity (space)
App->physics->SetGravity(0.0f, 0.0f);
```

#### SetDebugMode / IsDebugMode
```cpp
void SetDebugMode(bool enabled)
bool IsDebugMode()
```
Enable/disable debug visualization of physics shapes.
- **Note**: Press F1 at runtime to toggle debug mode
- **Example**:
```cpp
App->physics->SetDebugMode(true);  // Show all physics bodies
```

### Raycasting

#### Raycast
```cpp
bool Raycast(float x1, float y1, float x2, float y2, PhysBody*& hitBody, float& hitX, float& hitY, float& hitNormalX, float& hitNormalY)
```
Cast a ray and detect the first intersection.
- **Returns**: true if something was hit
- **Example**:
```cpp
PhysBody* hitBody;
float hitX, hitY, hitNX, hitNY;

if (App->physics->Raycast(100, 100, 500, 100, hitBody, hitX, hitY, hitNX, hitNY))
{
    LOG("Hit body at (%.1f, %.1f), normal: (%.2f, %.2f)", hitX, hitY, hitNX, hitNY);
    // Draw impact effect at hit point
}
```

#### QueryArea
```cpp
int QueryArea(float minX, float minY, float maxX, float maxY, std::vector<PhysBody*>& outBodies)
```
Find all bodies within a rectangular area.
- **Returns**: Number of bodies found
- **Example**:
```cpp
std::vector<PhysBody*> bodiesInArea;
int count = App->physics->QueryArea(200, 200, 600, 600, bodiesInArea);

LOG("Found %d bodies in area", count);
for (PhysBody* body : bodiesInArea)
{
    // Process each body
}
```

## PhysBody API

### Position & Rotation

```cpp
// Get position
void GetPosition(int& x, int& y);        // Returns integer pixels
void GetPositionF(float& x, float& y);   // Returns float pixels

// Set position
void SetPosition(float x, float y);

// Get rotation (in degrees)
float GetRotation();

// Set rotation (in degrees)
void SetRotation(float degrees);
```

**Example**:
```cpp
float x, y;
ball->GetPositionF(x, y);
LOG("Ball at (%.1f, %.1f)", x, y);

ball->SetPosition(400, 300);  // Teleport to center
ball->SetRotation(45.0f);     // Rotate 45 degrees
```

### Velocity

```cpp
// Linear velocity (pixels/second)
void GetLinearVelocity(float& vx, float& vy);
void SetLinearVelocity(float vx, float vy);

// Angular velocity (degrees/second)
float GetAngularVelocity();
void SetAngularVelocity(float omega);
```

**Example**:
```cpp
// Launch projectile
ball->SetLinearVelocity(500, -300);  // 500 px/s right, 300 px/s up

// Stop movement
ball->SetLinearVelocity(0, 0);

// Spin the body
ball->SetAngularVelocity(180.0f);  // 180 degrees/second
```

### Forces & Impulses

```cpp
// Apply force (gradual acceleration)
void ApplyForce(float fx, float fy);
void ApplyForceAtPoint(float fx, float fy, float pointX, float pointY);

// Apply impulse (instant velocity change)
void ApplyLinearImpulse(float ix, float iy);
void ApplyLinearImpulseAtPoint(float ix, float iy, float pointX, float pointY);

// Torque (rotational force)
void ApplyTorque(float torque);
void ApplyAngularImpulse(float impulse);
```

**Example**:
```cpp
// Continuous thrust (like a rocket)
if (IsKeyDown(KEY_SPACE))
{
    player->ApplyForce(0, -500);  // Upward force
}

// Jump (instant velocity)
if (IsKeyPressed(KEY_SPACE))
{
    player->ApplyLinearImpulse(0, -300);  // Instant upward boost
}

// Apply force at specific point (causes rotation)
paddle->ApplyForceAtPoint(1000, 0, paddleX + 50, paddleY);  // Force at edge
```

### Body Properties

```cpp
// Body type
enum class BodyType { STATIC, KINEMATIC, DYNAMIC };
void SetBodyType(BodyType type);
BodyType GetBodyType();

// Activation
void SetActive(bool active);
bool IsActive();

// Rotation lock
void SetFixedRotation(bool fixed);
bool IsFixedRotation();

// Gravity scale
void SetGravityScale(float scale);  // 0 = no gravity, 1 = normal, 2 = double
float GetGravityScale();
```

**Example**:
```cpp
// Make platform kinematic (movable but not affected by forces)
platform->SetBodyType(PhysBody::BodyType::KINEMATIC);

// Disable gravity for flying enemy
enemy->SetGravityScale(0.0f);

// Prevent rotation (for player character)
player->SetFixedRotation(true);

// Deactivate body (still exists but doesn't simulate)
body->SetActive(false);
```

### Physical Properties

```cpp
// Material properties
void SetDensity(float density);      // Affects mass
void SetFriction(float friction);    // 0.0 = ice, 1.0 = rough
void SetRestitution(float bounce);   // 0.0 = no bounce, 1.0 = perfect bounce

// Mass
float GetMass();       // Returns mass in kilograms
float GetInertia();    // Returns rotational mass
```

**Example**:
```cpp
// Bouncy ball
ball->SetRestitution(0.9f);
ball->SetDensity(0.5f);   // Light

// Heavy steel block
block->SetDensity(10.0f);
block->SetFriction(0.8f);
block->SetRestitution(0.1f);  // Minimal bounce

// Ice cube (slippery)
ice->SetFriction(0.05f);
```

### Collision Properties

```cpp
// Sensor (detects collisions but doesn't physically collide)
void SetSensor(bool isSensor);
bool IsSensor();

// Collision filtering
void SetCategoryBits(unsigned short category);  // What I am
void SetMaskBits(unsigned short mask);          // What I collide with
void SetGroupIndex(short group);                // Group collision rules
```

**Example**:
```cpp
// Create trigger zone (sensor)
PhysBody* trigger = App->physics->CreateCircle(400, 300, 100, PhysBody::BodyType::STATIC);
trigger->SetSensor(true);  // Doesn't block, just detects

// Collision categories
#define CATEGORY_PLAYER  0x0001
#define CATEGORY_ENEMY   0x0002
#define CATEGORY_BULLET  0x0004
#define CATEGORY_WALL    0x0008

player->SetCategoryBits(CATEGORY_PLAYER);
player->SetMaskBits(CATEGORY_ENEMY | CATEGORY_WALL);  // Collides with enemies and walls only

bullet->SetCategoryBits(CATEGORY_BULLET);
bullet->SetMaskBits(CATEGORY_ENEMY | CATEGORY_WALL);  // Bullets hit enemies and walls
```

### User Data

```cpp
void SetUserData(void* data);
void* GetUserData();
```

**Example**:
```cpp
// Store game entity pointer
struct Enemy {
    int health;
    PhysBody* body;
};

Enemy* enemy = new Enemy();
enemy->health = 100;
enemy->body = App->physics->CreateCircle(200, 200, 30, PhysBody::BodyType::DYNAMIC);
enemy->body->SetUserData(enemy);  // Link physics body to game entity

// Later, retrieve game entity from physics body
Enemy* e = (Enemy*)physBody->GetUserData();
e->health -= 10;
```

### Collision Callbacks

```cpp
class CollisionListener
{
    virtual void OnCollisionEnter(PhysBody* other) {}
    virtual void OnCollisionExit(PhysBody* other) {}
    virtual void OnCollisionStay(PhysBody* other) {}
};

void SetCollisionListener(CollisionListener* listener);
CollisionListener* GetCollisionListener();
```

**Example**:
```cpp
class Player : public PhysBody::CollisionListener
{
public:
    PhysBody* body;
    int health = 100;
    
    void OnCollisionEnter(PhysBody* other) override
    {
        Enemy* enemy = (Enemy*)other->GetUserData();
        if (enemy)
        {
            health -= 10;
            LOG("Player hit by enemy! Health: %d", health);
        }
    }
    
    void OnCollisionExit(PhysBody* other) override
    {
        LOG("Stopped touching body");
    }
};

// Setup
Player* player = new Player();
player->body = App->physics->CreateCircle(100, 100, 25, PhysBody::BodyType::DYNAMIC);
player->body->SetCollisionListener(player);  // Register for callbacks
```

## Complete Example: Pinball Game

```cpp
class ModuleGame : public Module
{
private:
    PhysBody* ball;
    PhysBody* leftFlipper;
    PhysBody* rightFlipper;
    PhysBody* walls;
    
public:
    bool Start() override
    {
        // Create ball
        ball = App->physics->CreateCircle(400, 100, 15, PhysBody::BodyType::DYNAMIC);
        ball->SetRestitution(0.7f);
        ball->SetDensity(1.0f);
        
        // Create flippers (simplified - use polygons for real flippers)
        leftFlipper = App->physics->CreateRectangle(300, 550, 80, 20, PhysBody::BodyType::KINEMATIC);
        rightFlipper = App->physics->CreateRectangle(500, 550, 80, 20, PhysBody::BodyType::KINEMATIC);
        
        // Create walls (chain around playfield)
        float wallVertices[] = {
            50, 50,
            750, 50,
            750, 600,
            50, 600
        };
        walls = App->physics->CreateChain(0, 0, wallVertices, 4, true);  // Loop
        
        return true;
    }
    
    update_status Update() override
    {
        // Control flippers
        if (IsKeyPressed(KEY_LEFT_SHIFT))
        {
            leftFlipper->ApplyAngularImpulse(-50);  // Rotate counter-clockwise
        }
        
        if (IsKeyPressed(KEY_RIGHT_SHIFT))
        {
            rightFlipper->ApplyAngularImpulse(50);  // Rotate clockwise
        }
        
        // Draw ball
        float x, y;
        ball->GetPositionF(x, y);
        DrawCircle((int)x, (int)y, 15, RED);
        
        return UPDATE_CONTINUE;
    }
    
    bool CleanUp() override
    {
        // No need to destroy bodies - ModulePhysics handles cleanup automatically
        return true;
    }
};
```

## Best Practices

1. **NEVER include box2d.h in game code** - Use only ModulePhysics and PhysBody
2. **Use appropriate body types**:
   - STATIC for walls, ground, obstacles
   - DYNAMIC for moving objects affected by physics
   - KINEMATIC for player-controlled or animated objects
3. **Set nullptr after destroying bodies**:
   ```cpp
   App->physics->DestroyBody(myBody);
   myBody = nullptr;
   ```
4. **Use sensors for trigger zones** - Don't physically block, just detect
5. **Apply impulses for instant effects** (jumping, explosions)
6. **Apply forces for continuous effects** (thrust, wind)
7. **Use collision filtering** to optimize performance and control what collides
8. **Store game entity pointers** in UserData for easy access during collisions
9. **Units**: All positions/sizes in pixels, velocities in pixels/second, forces in Newtons

## Troubleshooting

**Bodies falling through each other?**
- Decrease velocity (bodies moving too fast)
- Increase physics substeps in ModulePhysics::PreUpdate

**Bodies not colliding?**
- Check collision masks (SetCategoryBits/SetMaskBits)
- Ensure bodies are active (IsActive())
- Check if body is a sensor

**Physics feels wrong?**
- Adjust gravity: `App->physics->SetGravity(0, 980)`
- Tune restitution (bounciness)
- Tune density (mass)
- Tune friction

**Performance issues?**
- Use STATIC bodies for non-moving objects
- Use collision filtering to reduce unnecessary collision checks
- Deactivate bodies that are off-screen
- Limit number of dynamic bodies

## Conversion Constants

- **METERS_TO_PIXELS**: 50 (1 meter = 50 pixels)
- **PIXELS_TO_METERS**: 0.02 (1 pixel = 0.02 meters)
- **Earth gravity**: ~980 pixels/s² (9.8 m/s²)
- **Typical character speed**: 100-300 pixels/second
- **Typical jump impulse**: -200 to -400 pixels/second
