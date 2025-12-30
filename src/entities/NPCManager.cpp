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
#include <box2d/box2d.h>

// Static map to track each NPC's checkpoint progress
static std::map<Car*, int> npcTargetIndices;

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

	// Create the three NPC cars
	CreateNPC("NPC1", "assets/sprites/npc_1.png");
	CreateNPC("NPC2", "assets/sprites/npc_2.png");
	CreateNPC("NPC3", "assets/sprites/npc_3.png");

	LOG("NPC Manager initialized - %d NPCs created", (int)npcCars.size());
	return true;
}

update_status NPCManager::Update()
{
	// Update all NPC cars with AI
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

// Helper para saber si un cuerpo es un obstáculo real (pared) y no un sensor (checkpoint/meta)
bool IsRealObstacle(PhysBody* body) {
    if (!body || !body->GetB2Body()) return false;
    
    // Si no es estático, no es una pared del mapa (asumimos que solo esquivamos estáticos por ahora)
    if (body->GetB2Body()->GetType() != b2_staticBody) return false;

    // Comprobar si es un sensor (los checkpoints son sensores estáticos, hay que ignorarlos)
    const b2Fixture* fixture = body->GetB2Body()->GetFixtureList();
    while (fixture) {
        if (fixture->IsSensor()) return false; // Es un sensor, NO es un obstáculo
        fixture = fixture->GetNext();
    }

    return true; // Es estático y sólido
}

void NPCManager::UpdateAI(Car* npc)
{
    if (!npc || !App->checkpointManager) return;

    // 1. INICIALIZAR PROGRESO DEL NPC (Si es la primera vez)
    if (npcTargetIndices.find(npc) == npcTargetIndices.end()) {
        npcTargetIndices[npc] = 1; // Todos empiezan yendo al Checkpoint 1
    }

    int currentTarget = npcTargetIndices[npc];
    float npcX, npcY;
    npc->GetPosition(npcX, npcY);
    float npcAngle = npc->GetRotation();

    // 2. GESTIÓN DE CHECKPOINTS (Pathfinding Básico)
    float targetX, targetY;
    
    // Intentamos obtener la posición del checkpoint objetivo de este NPC
    if (!App->checkpointManager->GetCheckpointPosition(currentTarget, targetX, targetY)) {
        // Fallback: use a default position if checkpoint not found
        targetX = 2714.0f; // C1 position as example
        targetY = 1472.0f;
    }

    // Comprobar distancia al objetivo
    float dx = targetX - npcX;
    float dy = targetY - npcY;
    float distToTarget = sqrtf(dx*dx + dy*dy);

    // Si estamos cerca del checkpoint (ej. 300 pixeles), pasar al siguiente
    if (distToTarget < 300.0f) {
        npcTargetIndices[npc]++; // ¡Bien hecho NPC! Ve al siguiente.
        if (npcTargetIndices[npc] > App->checkpointManager->GetTotalCheckpoints()) {
            npcTargetIndices[npc] = 0; // Go to finish line
        }
        
        // Actualizamos el objetivo inmediatamente para que 
        // el coche siga conduciendo hacia el nuevo punto en este mismo frame.
        currentTarget = npcTargetIndices[npc];
        if (!App->checkpointManager->GetCheckpointPosition(currentTarget, targetX, targetY)) {
             // Mantener fallback o última posición conocida si falla
        }
        // LOG("NPC reached checkpoint %d, moving to %d", currentTarget - 1, currentTarget);
    }

    // Recalcular targetX/Y por si ha cambiado el objetivo arriba
    if (App->checkpointManager->GetCheckpointPosition(currentTarget, targetX, targetY)) {
        dx = targetX - npcX;
        dy = targetY - npcY;
        // distToTarget = sqrtf(dx*dx + dy*dy); // No necesitamos recalcular dist exacta para el ángulo
    }

    // 3. EVASIÓN DE OBSTÁCULOS (RAYCASTS / BIGOTES)
    
    // Definir los bigotes (longitud y ángulo)
    float rayLength = 250.0f; 
    float rayAngle = 30.0f;   
    
    float angleRad = (npcAngle - 90.0f) * (PI / 180.0f); 
    
    vec2f forward = { cosf(angleRad), sinf(angleRad) };
    vec2f leftRay = { cosf(angleRad - rayAngle * (PI/180.0f)), sinf(angleRad - rayAngle * (PI/180.0f)) };
    vec2f rightRay = { cosf(angleRad + rayAngle * (PI/180.0f)), sinf(angleRad + rayAngle * (PI/180.0f)) };

    PhysBody* hitBody;
    float hitX, hitY, nX, nY;
    
    float avoidSteer = 0.0f;
    bool obstacleDetected = false;

    // --- RAYO IZQUIERDO ---
    if (App->physics->Raycast(npcX, npcY, npcX + leftRay.x * rayLength, npcY + leftRay.y * rayLength, hitBody, hitX, hitY, nX, nY)) {
        if (IsRealObstacle(hitBody)) { // FIX: Usar función auxiliar que ignora sensores
            avoidSteer += 1.0f; 
            obstacleDetected = true;
        }
    }

    // --- RAYO DERECHO ---
    if (App->physics->Raycast(npcX, npcY, npcX + rightRay.x * rayLength, npcY + rightRay.y * rayLength, hitBody, hitX, hitY, nX, nY)) {
        if (IsRealObstacle(hitBody)) {
            avoidSteer -= 1.0f; 
            obstacleDetected = true;
        }
    }

    // --- RAYO CENTRAL (Frenado de emergencia) ---
    float centerBrake = 0.0f;
    if (App->physics->Raycast(npcX, npcY, npcX + forward.x * (rayLength + 50), npcY + forward.y * (rayLength + 50), hitBody, hitX, hitY, nX, nY)) {
        if (IsRealObstacle(hitBody)) {
            centerBrake = 1.0f; // ¡Muro defrente real! FRENAR
            obstacleDetected = true;
             if (avoidSteer == 0.0f) avoidSteer = 1.0f; 
        }
    }

    // 4. DECISIÓN FINAL DE DIRECCIÓN
    float finalSteer = 0.0f;
    float finalAccel = 0.0f;
    float finalBrake = 0.0f;

    if (obstacleDetected) {
        // MODO PÁNICO
        finalSteer = avoidSteer;
        finalAccel = 0.3f; 
        if (centerBrake > 0.0f) {
            finalAccel = 0.0f;
            finalBrake = 0.5f; 
            finalSteer = 1.0f; 
        }
    } else {
        // MODO CARRERA
        float desiredAngleRad = atan2f(targetY - npcY, targetX - npcX);
        float desiredAngleDeg = desiredAngleRad * (180.0f / PI);
        desiredAngleDeg += 90.0f; 

        float angleDiff = desiredAngleDeg - npcAngle;
        while (angleDiff <= -180) angleDiff += 360;
        while (angleDiff > 180) angleDiff -= 360;

        if (angleDiff > 10.0f) finalSteer = 1.0f;
        else if (angleDiff < -10.0f) finalSteer = -1.0f;
        else finalSteer = angleDiff / 10.0f; 

        // Gestión de velocidad en curvas (ligeramente ajustada para ser menos miedosa)
        if (fabs(angleDiff) < 20.0f) finalAccel = 1.0f; 
        else if (fabs(angleDiff) > 60.0f) { finalAccel = 0.3f; finalBrake = 0.0f; } // No frenar en seco, solo soltar gas
        else finalAccel = 0.8f; 
    }

    // 5. APLICAR INPUTS
    npc->Steer(finalSteer);
    npc->Accelerate(finalAccel);
    if (finalBrake > 0.0f) npc->Brake(finalBrake);
}

update_status NPCManager::PostUpdate()
{
	// Draw all NPC cars
	for (Car* npc : npcCars)
	{
		if (npc)
		{
			npc->Draw();
		}
	}

	return UPDATE_CONTINUE;
}

bool NPCManager::CleanUp()
{
	LOG("Cleaning up NPC Manager");

	// Cleanup all NPC cars
	for (Car* npc : npcCars)
	{
		if (npc)
		{
			delete npc;
		}
	}
	npcCars.clear();

	return true;
}

void NPCManager::CreateNPC(const char* npcName, const char* texturePath)
{
	LOG("Creating NPC: %s", npcName);

	// Create the NPC car
	Car* npcCar = new Car(App);
	if (!npcCar->Start())
	{
		LOG("ERROR: Failed to create NPC car: %s", npcName);
		delete npcCar;
		return;
	}

	// CRITICAL: Get starting position from Tiled map
	// The "Start" objects with Name="NPC1/2/3" are in the "Positions" layer
	// Layer offset: offsetx="1664" offsety="984"

	// Find the Start object with Name property matching this NPC
	MapObject* startPos = nullptr;
	for (const auto& object : App->map->mapData.objects)
	{
		if (object->name == "Start")
		{
			// Check if this object has the "Name" property set to this NPC name
			Properties::Property* nameProp = object->properties.GetProperty("Name");
			if (nameProp && nameProp->value == npcName)
			{
				startPos = object;
				break;
			}
		}
	}

	if (startPos)
	{
		// CRITICAL FIX: The "Positions" layer in Tiled has an offset!
		// offsetx="1664" offsety="984" (from Map.tmx)
		// We need to ADD this offset to the object's position
		const float POSITIONS_LAYER_OFFSET_X = 1664.0f;
		const float POSITIONS_LAYER_OFFSET_Y = 984.0f;

		// Apply layer offset to get world coordinates
		float worldX = (float)startPos->x + POSITIONS_LAYER_OFFSET_X;
		float worldY = (float)startPos->y + POSITIONS_LAYER_OFFSET_Y;

		LOG("Start position found for %s at Tiled coords: (%d, %d)", npcName, startPos->x, startPos->y);
		LOG("With layer offset applied: (%.2f, %.2f)", worldX, worldY);

		// Set NPC car position
		npcCar->SetPosition(worldX, worldY);

		// Set starting rotation (same as player: 270 = facing down)
		npcCar->SetRotation(270.0f);

		LOG("NPC car %s positioned at (%.2f, %.2f) with rotation 270", npcName, worldX, worldY);
	}
	else
	{
		LOG("Warning: No start position found for %s in map, using default", npcName);
		// Fallback to default position (offset to avoid overlap)
		float defaultX = 500.0f + ((float)npcCars.size() * 100.0f);
		npcCar->SetPosition(defaultX, 300.0f);
		npcCar->SetRotation(270.0f);
	}

	// Configure NPC car with IDENTICAL properties to player
	// CRITICAL: Same dimensions, speed, and capabilities as player car
	npcCar->SetMaxSpeed(800.0f);          // Same as player
	npcCar->SetReverseSpeed(400.0f);      // Same as player
	npcCar->SetAcceleration(20.0f);       // Default acceleration
	npcCar->SetSteeringSensitivity(200.0f); // Default steering

	// Load NPC texture
	Texture2D npcTexture = App->resources->LoadTexture(texturePath);
	if (npcTexture.id != 0)
	{
		npcCar->SetTexture(npcTexture);
		LOG("NPC %s texture loaded: %s", npcName, texturePath);
	}
	else
	{
		LOG("WARNING: Failed to load NPC %s texture: %s", npcName, texturePath);
		// Set a distinct color as fallback for each NPC
		if (strcmp(npcName, "NPC1") == 0)
			npcCar->SetColor(RED);
		else if (strcmp(npcName, "NPC2") == 0)
			npcCar->SetColor(GREEN);
		else if (strcmp(npcName, "NPC3") == 0)
			npcCar->SetColor(YELLOW);
	}

	// Add to NPC list
	npcCars.push_back(npcCar);

	LOG("NPC car %s created successfully", npcName);
}

Car* NPCManager::GetNPC(int index) const
{
	if (index >= 0 && index < (int)npcCars.size())
		return npcCars[index];
	return nullptr;
}