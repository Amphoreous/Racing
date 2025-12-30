#include "entities/NPCManager.h"
#include "core/Application.h"
#include "core/Map.h"
#include "entities/Car.h"
#include "entities/CheckpointManager.h"
#include "modules/ModuleResources.h"
#include "modules/ModulePhysics.h"
#include "core/p2Point.h"
#include "entities/PushAbility.h"
#include "entities/Player.h"
#include "raylib.h"
#include <cmath>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <box2d/box2d.h>

// Estructura para almacenar la info de cada rayo del "Radar"
struct RaySensor {
    float angleOffset; // Grados relativos al morro (-45, 0, 45, etc)
    float distance;    // Distancia detectada
    bool hit;          // Si chocó con algo
};

struct NPCState {
    int targetIndex;
    std::string stateName;

    // Para visualización debug
    std::vector<RaySensor> sensors;
    int bestRayIndex;

    // Detección de atascos
    bool stuck;
    float stuckTimer;
    float reverseSteerDir;

    // Para detección de uso de habilidad
    float lastAbilityCheck;  // Temporizador para no chequear cada frame
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

    // Clean up existing resources before creating new ones
    // This prevents memory leaks when Start() is called multiple times (Enable/Disable cycles)
    if (!npcAbilities.empty())
    {
        LOG("NPC abilities already exist - cleaning up before re-creation");
        for (PushAbility* ability : npcAbilities)
        {
            if (ability) delete ability;
        }
        npcAbilities.clear();
    }
    if (!npcCars.empty())
    {
        LOG("NPC cars already exist - cleaning up before re-creation");
        for (Car* npc : npcCars)
        {
            if (npc) delete npc;
        }
        npcCars.clear();
    }
    // Clear NPC states map too
    npcStates.clear();

    CreateNPC("NPC1", "assets/sprites/npc_1.png");
    CreateNPC("NPC2", "assets/sprites/npc_2.png");
    CreateNPC("NPC3", "assets/sprites/npc_3.png");

    // Initialize abilities for each NPC (one per car)
    for (size_t i = 0; i < npcCars.size(); i++)
    {
        PushAbility* ability = new PushAbility();
        if (ability->Init(App, false))  // false -> NPC ability (no cooldown-ready sound)
        {
            npcAbilities.push_back(ability);
            LOG("NPC%d ability initialized", (int)i + 1);
        }
        else
        {
            delete ability;
            npcAbilities.push_back(nullptr);
            LOG("WARNING: Failed to init ability for NPC%d", (int)i + 1);
        }
    }

    return true;
}

update_status NPCManager::Update()
{
    for (size_t i = 0; i < npcCars.size(); i++)
    {
        Car* npc = npcCars[i];
        PushAbility* ability = (i < npcAbilities.size()) ? npcAbilities[i] : nullptr;

        if (npc)
        {
            UpdateAI(npc);
            npc->Update();

            // Check and use ability
            if (ability)
            {
                ability->Update();
                CheckAndUseAbility(npc, ability);
            }
        }
    }
    return UPDATE_CONTINUE;
}

// Helper: Detecta solo muros estáticos reales
bool IsRealObstacle(PhysBody* body) {
    if (!body || !body->GetB2Body()) return false;
    if (body->GetB2Body()->GetType() != b2_staticBody) return false;

    // Ignorar sensores (checkpoints)
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

    // Inicializar estado
    if (npcStates.find(npc) == npcStates.end()) {
        npcStates[npc] = { 1, "INIT", {}, 2, false, 0.0f, 0.0f, 0.0f };
        // Definir los 5 sensores del radar (ángulos en grados)
        // Cubrimos un abanico amplio para "ver" las curvas cerradas
        npcStates[npc].sensors = {
            { -60.0f, 0.0f, false }, // Izquierda Extrema
            { -30.0f, 0.0f, false }, // Izquierda Diagonal
            {   0.0f, 0.0f, false }, // Centro
            {  30.0f, 0.0f, false }, // Derecha Diagonal
            {  60.0f, 0.0f, false }  // Derecha Extrema
        };
    }
    NPCState& state = npcStates[npc];
    float dt = 1.0f / 60.0f;

    float npcX, npcY;
    npc->GetPosition(npcX, npcY);
    float npcAngle = npc->GetRotation(); // 0..360
    float npcAngleRad = (npcAngle - 90.0f) * (PI / 180.0f); // Box2D offset

    // --- 1. GESTIÓN DE CHECKPOINTS ---
    float targetX, targetY;
    if (!App->checkpointManager->GetCheckpointPosition(state.targetIndex, targetX, targetY)) {
        targetX = 2714.0f; targetY = 1472.0f; 
    }

    float dx = targetX - npcX;
    float dy = targetY - npcY;
    float distToTarget = sqrtf(dx*dx + dy*dy);

    // Radio de aceptación amplio para fluidez
    if (distToTarget < 400.0f) {
        state.targetIndex++;
        if (state.targetIndex > App->checkpointManager->GetTotalCheckpoints()) {
            state.targetIndex = 0; 
        }
        state.stuckTimer = 0; 
    }

    // --- 2. RADAR (Gap Finding) ---
    float maxViewDistance = 450.0f;
    PhysBody* hitBody;
    float hitX, hitY, nX, nY;

    // Actualizar todos los sensores
    for (auto& sensor : state.sensors) {
        float rayAngleRad = npcAngleRad + (sensor.angleOffset * (PI / 180.0f));
        vec2f dir = { cosf(rayAngleRad), sinf(rayAngleRad) };
        
        // Raycast
        bool hit = App->physics->Raycast(npcX, npcY, npcX + dir.x * maxViewDistance, npcY + dir.y * maxViewDistance, hitBody, hitX, hitY, nX, nY);
        
        if (hit && IsRealObstacle(hitBody)) {
            // Calcular distancia real al impacto
            float hdx = hitX - npcX;
            float hdy = hitY - npcY;
            sensor.distance = sqrtf(hdx*hdx + hdy*hdy);
            sensor.hit = true;
        } else {
            sensor.distance = maxViewDistance; // Camino libre
            sensor.hit = false;
        }
    }

    // --- 3. EVALUACIÓN DE DIRECCIONES (El Cerebro) ---
    
    // Calcular ángulo hacia el objetivo en coordenadas locales del coche
    float absTargetAngleRad = atan2f(targetY - npcY, targetX - npcX);
    float absTargetAngleDeg = absTargetAngleRad * (180.0f / PI) + 90.0f; // Ajuste mundo
    
    // Diferencia relativa (-180 a 180) entre morro y objetivo
    float relativeTargetAngle = absTargetAngleDeg - npcAngle;
    while (relativeTargetAngle <= -180) relativeTargetAngle += 360;
    while (relativeTargetAngle > 180) relativeTargetAngle -= 360;

    // Buscar el mejor rayo
    float bestScore = -99999.0f;
    int bestIndex = 2; // Default centro

    for (int i = 0; i < (int)state.sensors.size(); i++) {
        float score = 0.0f;
        RaySensor& s = state.sensors[i];

        // A. Puntuación base por espacio libre (Esencial para no chocar)
        // Normalizamos distancia (0.0 a 1.0)
        float spaceScore = s.distance / maxViewDistance;
        
        // Si la distancia es muy crítica (< 50px), penalización masiva (Muro inminente)
        if (s.distance < 60.0f) spaceScore = -10.0f; 

        score += spaceScore * 2.0f; // Peso alto a no chocar

        // B. Puntuación por alineación con el objetivo (Pathfinding)
        // ¿Cuánto se alinea este rayo con el objetivo?
        float rayRelAngle = s.angleOffset; // -60, -30, 0...
        float angleDiff = fabs(relativeTargetAngle - rayRelAngle);
        
        // Cuanto menor la diferencia, mejor (Bonus de hasta 1.0 punto)
        // Solo aplicamos este bonus si el camino es transitable (> 100px)
        if (s.distance > 100.0f) {
            float alignBonus = (180.0f - angleDiff) / 180.0f; 
            score += alignBonus * 1.5f; // Peso medio al objetivo
        }

        if (score > bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }

    state.bestRayIndex = bestIndex; // Guardamos para debug

    // --- 4. DETECCIÓN DE ATASCO (STUCK) ---
    float speed = npc->GetCurrentSpeed();
    // Si vamos lento pero NO estamos frenando voluntariamente (por muro frontal lejano)
    if (speed < 10.0f) {
        state.stuckTimer += dt;
    } else {
        if (!state.stuck) state.stuckTimer = 0.0f;
    }

    if (state.stuckTimer > 2.0f && !state.stuck) {
        state.stuck = true;
        // Invertimos dirección de escape
        state.reverseSteerDir = (GetRandomValue(0, 1) == 0) ? 1.0f : -1.0f;
    }

    // --- 5. INPUTS FINALES ---
    float finalSteer = 0.0f;
    float finalAccel = 0.0f;
    float finalBrake = 0.0f;

    if (state.stuck) {
        state.stateName = "STUCK - REVERSE";
        finalAccel = -1.0f; 
        finalSteer = state.reverseSteerDir;
        if (state.stuckTimer > 3.5f) { // 1.5s de maniobra
            state.stuck = false;
            state.stuckTimer = 0.0f;
        }
    } 
    else {
        // MODO CONDUCCIÓN POR "HUECOS"
        
        // El índice 2 es centro (0 grados). 
        // 0 (-60 deg) -> Steer -1.0 (Left)
        // 4 (+60 deg) -> Steer +1.0 (Right)
        // Mapeamos el índice elegido (-2 a +2) a dirección (-1.0 a 1.0)
        float steerDir = (float)(bestIndex - 2) / 2.0f; 
        
        // Suavizado: Si el mejor rayo es el central, intentamos afinar más hacia el objetivo
        if (bestIndex == 2) {
            if (relativeTargetAngle > 5.0f) steerDir = 0.2f;
            else if (relativeTargetAngle < -5.0f) steerDir = -0.2f;
        }

        finalSteer = steerDir;
        state.stateName = "SEEKING GAP";

        // Gestión de velocidad inteligente
        float centerDist = state.sensors[2].distance;
        
        // Si el rayo central es corto, frenar para poder girar
        if (centerDist < 150.0f) {
            finalAccel = 0.2f; // Poco gas
            // Si vamos muy rápido, freno
            if (speed > 400.0f) finalBrake = 0.5f; 
            state.stateName = "TIGHT CORNER";
        } 
        else if (fabs(finalSteer) > 0.6f) {
            // Giro cerrado pero con espacio (Drifting)
            finalAccel = 0.6f;
        }
        else {
            // Recta o curva suave
            finalAccel = 1.0f;
        }
    }

    // Aplicar
    npc->Steer(finalSteer);
    npc->Accelerate(finalAccel);
    if (finalAccel < 0.0f) npc->Reverse(fabs(finalAccel));
    if (finalBrake > 0.0f) npc->Brake(finalBrake);
}

update_status NPCManager::PostUpdate()
{
    for (size_t i = 0; i < npcCars.size(); i++)
    {
        Car* npc = npcCars[i];
        PushAbility* ability = (i < npcAbilities.size()) ? npcAbilities[i] : nullptr;

        if (npc)
        {
            npc->Draw();

            // Draw ability effect
            if (ability)
            {
                ability->Draw();
            }

            if (App->physics->IsDebugMode() && npcStates.find(npc) != npcStates.end()) {
                NPCState& state = npcStates[npc];
                float x, y;
                npc->GetPosition(x, y);

                float angle = npc->GetRotation();
                float angleRad = (angle - 90.0f) * (PI / 180.0f);
                Vector2 center = { x, y };

                for (int j = 0; j < (int)state.sensors.size(); j++) {
                    RaySensor& s = state.sensors[j];
                    float rayA = angleRad + s.angleOffset * (PI / 180.0f);

                    float visLen = s.distance;
                    Vector2 end = { x + cosf(rayA) * visLen, y + sinf(rayA) * visLen };

                    Color col = (s.hit) ? RED : GREEN;
                    if (j == state.bestRayIndex) col = WHITE;

                    DrawLineV(center, end, col);

                    if (j == state.bestRayIndex) {
                        DrawLineV({ center.x + 1, center.y }, { end.x + 1, end.y }, col);
                    }
                }
            }
        }
    }

    return UPDATE_CONTINUE;
}

bool NPCManager::CleanUp()
{
    LOG("Cleaning up NPC Manager");

    for (PushAbility* ability : npcAbilities)
    {
        if (ability) delete ability;
    }
    npcAbilities.clear();

    // Cleanup cars
    for (Car* npc : npcCars) if (npc) delete npc;
    npcCars.clear();

    npcStates.clear();
    return true;
}

void NPCManager::CheckAndUseAbility(Car* npc, PushAbility* ability)
{
    if (!npc || !ability) return;

    // Check ability every 0.5 seconds (optimization)
    NPCState& state = npcStates[npc];
    state.lastAbilityCheck += GetFrameTime();

    if (state.lastAbilityCheck < 0.5f) return;
    state.lastAbilityCheck = 0.0f;

    // Only use ability if it's ready
    if (!ability->IsReady()) return;

    // Get NPC position
    float npcX, npcY;
    npc->GetPosition(npcX, npcY);

    const float DETECTION_RADIUS = 200.0f;  // Distance to detect other cars
    bool shouldUseAbility = false;

    // Check distance to player
    if (App->player && App->player->GetCar())
    {
        float playerX, playerY;
        App->player->GetCar()->GetPosition(playerX, playerY);

        float dx = playerX - npcX;
        float dy = playerY - npcY;
        float distToPlayer = sqrtf(dx * dx + dy * dy);

        if (distToPlayer < DETECTION_RADIUS)
        {
            shouldUseAbility = true;
        }
    }

    // Check distance to other NPCs (if not already triggered)
    if (!shouldUseAbility)
    {
        for (Car* otherNPC : npcCars)
        {
            if (otherNPC == npc || !otherNPC) continue;

            float otherX, otherY;
            otherNPC->GetPosition(otherX, otherY);

            float dx = otherX - npcX;
            float dy = otherY - npcY;
            float distToOther = sqrtf(dx * dx + dy * dy);

            if (distToOther < DETECTION_RADIUS)
            {
                shouldUseAbility = true;
                break;
            }
        }
    }

    // Use ability if someone is nearby
    if (shouldUseAbility)
    {
        float npcRotation = npc->GetRotation();
        // IMPORTANT: pass the activating car so the ability can exclude it from being pushed
        ability->Activate(npcX, npcY, npcRotation, npc);
        LOG("NPC used push ability!");
    }
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

	npcCar->SetMaxSpeed(1100.0f);          
	npcCar->SetReverseSpeed(500.0f);      
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