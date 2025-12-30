#include "entities/NPCManager.h"
#include "core/Application.h"
#include "core/Map.h"
#include "entities/Car.h"
#include "entities/CheckpointManager.h"
#include "modules/ModuleResources.h"
#include "modules/ModulePhysics.h"
#include "core/p2Point.h"
#include "raylib.h"
#include <cmath>
#include <map>
#include <string>
#include <box2d/box2d.h>

#ifndef PI
#define PI 3.141592653589793f
#endif

// Data structure to store AI state for debugging
struct NPCState {
    int targetIndex;
    std::string stateName;
    float debugRayLeftFactor; // 0.0 = clear, 1.0 = hit
    float debugRayRightFactor;
    float debugRayCenterFactor;
    bool stuck;
    float stuckTimer;
    float reverseSteerDir; // Direction to steer when reversing
};

static std::map<Car*, NPCState> npcStates;

NPCManager::NPCManager(Application* app, bool start_enabled)
	: Module(app, start_enabled)
{
}

NPCManager::~NPCManager()
{
}

bool NPCManager::Start()
{
	LOG("Creating NPC cars");
	CreateNPC("NPC1", "assets/sprites/npc_1.png");
	CreateNPC("NPC2", "assets/sprites/npc_2.png");
	CreateNPC("NPC3", "assets/sprites/npc_3.png");
	return true;
}

update_status NPCManager::Update()
{
	for (Car* npc : npcCars)
	{
		if (npc)
		{
			UpdateAI(npc);
			npc->Update();
		}
	}
	return UPDATE_CONTINUE;
}

// Helper to check if a body is a valid obstacle (Wall)
bool IsRealObstacle(PhysBody* body) {
    if (!body || !body->GetB2Body()) return false;
    
    // Ignore dynamic bodies (other cars)
    if (body->GetB2Body()->GetType() != b2_staticBody) return false;

    // Check if it's a sensor (Checkpoints are sensors)
    const b2Fixture* fixture = body->GetB2Body()->GetFixtureList();
    while (fixture) {
        if (fixture->IsSensor()) return false; 
        fixture = fixture->GetNext();
    }
    return true;
}

void NPCManager::UpdateAI(Car* npc)
{
    if (!npc || !App->checkpointManager) return;

    // Initialize state if new
    if (npcStates.find(npc) == npcStates.end()) {
        npcStates[npc] = { 1, "IDLE", 0.0f, 0.0f, 0.0f, false, 0.0f, 1.0f };
    }
    NPCState& state = npcStates[npc];

    float npcX, npcY;
    npc->GetPosition(npcX, npcY);
    float npcAngle = npc->GetRotation();

    // --- 1. TARGET MANAGEMENT ---
    float targetX, targetY;
    if (!App->checkpointManager->GetCheckpointPosition(state.targetIndex, targetX, targetY)) {
        targetX = 2714.0f; targetY = 1472.0f; 
    }

    float dx = targetX - npcX;
    float dy = targetY - npcY;
    float distToTarget = sqrtf(dx*dx + dy*dy);

    // FIX 1: Aumentado radio de aceptación a 500.0f para que no se obsesionen con el centro exacto
    // Si la "U" es muy cerrada, esto les permite pasar al siguiente punto antes de chocar.
    if (distToTarget < 500.0f) {
        state.targetIndex++;
        if (state.targetIndex > App->checkpointManager->GetTotalCheckpoints()) {
            state.targetIndex = 0; // Finish line
        }
        state.stuckTimer = 0; // Reset stuck timer on progress
    }

    // --- 2. RAYCASTS (WHISKERS) ---
    // Bigotes más anchos (45 grados) para ver mejor las curvas cerradas
    float rayLength = 320.0f; 
    float angleRad = (npcAngle - 90.0f) * (PI / 180.0f);
    
    vec2f forward = { cosf(angleRad), sinf(angleRad) };
    vec2f leftRay = { cosf(angleRad - 45.0f * (PI/180.0f)), sinf(angleRad - 45.0f * (PI/180.0f)) };
    vec2f rightRay = { cosf(angleRad + 45.0f * (PI/180.0f)), sinf(angleRad + 45.0f * (PI/180.0f)) };

    PhysBody* hitBody;
    float hitX, hitY, nX, nY;
    
    bool hitLeft = false;
    bool hitRight = false;
    bool hitCenter = false;

    state.debugRayLeftFactor = 0.0f;
    state.debugRayRightFactor = 0.0f;
    state.debugRayCenterFactor = 0.0f;

    if (App->physics->Raycast(npcX, npcY, npcX + leftRay.x * rayLength, npcY + leftRay.y * rayLength, hitBody, hitX, hitY, nX, nY)) {
        if (IsRealObstacle(hitBody)) { hitLeft = true; state.debugRayLeftFactor = 1.0f; }
    }
    if (App->physics->Raycast(npcX, npcY, npcX + rightRay.x * rayLength, npcY + rightRay.y * rayLength, hitBody, hitX, hitY, nX, nY)) {
        if (IsRealObstacle(hitBody)) { hitRight = true; state.debugRayRightFactor = 1.0f; }
    }
    if (App->physics->Raycast(npcX, npcY, npcX + forward.x * (rayLength + 60), npcY + forward.y * (rayLength + 60), hitBody, hitX, hitY, nX, nY)) {
        if (IsRealObstacle(hitBody)) { hitCenter = true; state.debugRayCenterFactor = 1.0f; }
    }

    // --- 3. STUCK DETECTION ---
    float speed = npc->GetCurrentSpeed();
    if (speed < 15.0f) {
        state.stuckTimer += 1.0f / 60.0f;
    } else {
        state.stuckTimer = 0.0f;
        state.stuck = false;
    }

    if (state.stuckTimer > 1.5f) { // Detect stuck faster (1.5s)
        if (!state.stuck) {
             state.stuck = true;
             // Pick a random direction to reverse into
             state.reverseSteerDir = (GetRandomValue(0, 1) == 0) ? 1.0f : -1.0f;
        }
    }

    // --- 4. DECISION MAKING ---
    float finalSteer = 0.0f;
    float finalAccel = 0.0f;
    float finalBrake = 0.0f;
    
    // Calculate Angle to Target for decision making
    float desiredAngleRad = atan2f(targetY - npcY, targetX - npcX);
    float desiredAngleDeg = desiredAngleRad * (180.0f / PI);
    desiredAngleDeg += 90.0f;
    float angleDiff = desiredAngleDeg - npcAngle;
    while (angleDiff <= -180) angleDiff += 360;
    while (angleDiff > 180) angleDiff -= 360;

    if (state.stuck) {
        state.stateName = "STUCK - REVERSING";
        finalAccel = -1.0f; // Reverse full power
        finalSteer = state.reverseSteerDir;
        
        // If we backed up enough, try going forward again
        if (state.stuckTimer > 3.0f) state.stuck = false; 
    }
    else if (hitCenter) {
        // WALL AHEAD
        state.stateName = "WALL AHEAD";
        
        // FIX 2: Evasión inteligente.
        // Si chocamos de frente, decidir giro basado en huecos O en el objetivo.
        if (hitLeft) {
            finalSteer = 1.0f; // Left blocked -> Go Right
        } else if (hitRight) {
            finalSteer = -1.0f; // Right blocked -> Go Left
        } else {
            // Both sides clear? Turn towards target!
            finalSteer = (angleDiff > 0) ? 1.0f : -1.0f;
        }

        // Only brake if really fast, otherwise power through the turn
        float vx, vy;
        npc->GetPhysBody()->GetLinearVelocity(vx, vy);
        float approachSpeed = vx * forward.x + vy * forward.y;
        
        if (approachSpeed > 150.0f) {
             finalBrake = 0.5f; // Soft brake
             finalAccel = 0.0f;
        } else {
             finalAccel = 0.5f; // Keep moving to turn
        }
    }
    else if (hitLeft) {
        state.stateName = "AVOID LEFT";
        finalSteer = 0.6f; // Turn Right
        finalAccel = 0.8f;
    }
    else if (hitRight) {
        state.stateName = "AVOID RIGHT";
        finalSteer = -0.6f; // Turn Left
        finalAccel = 0.8f;
    }
    else {
        state.stateName = "RACING";
        
        // Steer towards target
        if (angleDiff > 5.0f) finalSteer = 1.0f;
        else if (angleDiff < -5.0f) finalSteer = -1.0f;
        else finalSteer = angleDiff / 5.0f;

        // Speed logic
        if (fabs(angleDiff) < 20.0f) finalAccel = 1.0f; 
        else if (fabs(angleDiff) > 50.0f) finalAccel = 0.4f; // Slow down for sharp turns
        else finalAccel = 0.8f; 
    }

    npc->Steer(finalSteer);
    npc->Accelerate(finalAccel);
    if (finalAccel < 0.0f) npc->Reverse(fabs(finalAccel));
    if (finalBrake > 0.0f) npc->Brake(finalBrake);
}

update_status NPCManager::PostUpdate()
{
	for (Car* npc : npcCars)
	{
		if (npc)
		{
			npc->Draw();

            // --- VISUAL DEBUGGING ---
            if (npcStates.find(npc) != npcStates.end()) {
                NPCState& state = npcStates[npc];
                float x, y;
                npc->GetPosition(x, y);
                
                // Draw State Text
                DrawText(state.stateName.c_str(), (int)x - 20, (int)y - 50, 20, WHITE);
                
                // Debug Rays (Visual only)
                float angle = npc->GetRotation();
                float angleRad = (angle - 90.0f) * (PI / 180.0f);
                float rayLen = 320.0f;
                Vector2 center = { x, y };

                auto DrawDebugLine = [&](float angOffset, float hitFactor) {
                     float a = angleRad + angOffset * (PI/180.0f);
                     Vector2 end = { x + cosf(a)*rayLen, y + sinf(a)*rayLen };
                     Color col = (hitFactor > 0.0f) ? RED : GREEN;
                     DrawLineV(center, end, col);
                };
                
                DrawDebugLine(-45.0f, state.debugRayLeftFactor);
                DrawDebugLine(0.0f, state.debugRayCenterFactor);
                DrawDebugLine(45.0f, state.debugRayRightFactor);
            }
		}
	}

	return UPDATE_CONTINUE;
}

bool NPCManager::CleanUp()
{
	LOG("Cleaning up NPC Manager");
	for (Car* npc : npcCars) if (npc) delete npc;
	npcCars.clear();
    npcStates.clear();
	return true;
}

void NPCManager::CreateNPC(const char* npcName, const char* texturePath)
{
	LOG("Creating NPC: %s", npcName);

	Car* npcCar = new Car(App);
	if (!npcCar->Start()) { delete npcCar; return; }

	MapObject* startPos = nullptr;
	for (const auto& object : App->map->mapData.objects) {
		if (object->name == "Start") {
			Properties::Property* nameProp = object->properties.GetProperty("Name");
			if (nameProp && nameProp->value == npcName) {
				startPos = object;
				break;
			}
		}
	}

	if (startPos) {
		const float POSITIONS_LAYER_OFFSET_X = 1664.0f;
		const float POSITIONS_LAYER_OFFSET_Y = 984.0f;
		float worldX = (float)startPos->x + POSITIONS_LAYER_OFFSET_X;
		float worldY = (float)startPos->y + POSITIONS_LAYER_OFFSET_Y;
		npcCar->SetPosition(worldX, worldY);
		npcCar->SetRotation(270.0f);
	} else {
		float defaultX = 500.0f + ((float)npcCars.size() * 100.0f);
		npcCar->SetPosition(defaultX, 300.0f);
		npcCar->SetRotation(270.0f);
	}

	npcCar->SetMaxSpeed(800.0f);          
	npcCar->SetReverseSpeed(400.0f);      
	npcCar->SetAcceleration(20.0f);       
	npcCar->SetSteeringSensitivity(200.0f); 

	Texture2D npcTexture = App->resources->LoadTexture(texturePath);
	if (npcTexture.id != 0) npcCar->SetTexture(npcTexture);
	else {
		if (strcmp(npcName, "NPC1") == 0) npcCar->SetColor(RED);
		else if (strcmp(npcName, "NPC2") == 0) npcCar->SetColor(GREEN);
		else if (strcmp(npcName, "NPC3") == 0) npcCar->SetColor(YELLOW);
	}

	npcCars.push_back(npcCar);
}

Car* NPCManager::GetNPC(int index) const
{
	if (index >= 0 && index < (int)npcCars.size()) return npcCars[index];
	return nullptr;
}