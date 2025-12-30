#pragma once

#include "core/Module.h"
#include "core/Globals.h"
#include <vector>

class Car;
class PushAbility;  // ADDED: Forward declaration

class NPCManager : public Module
{
public:
	NPCManager(Application* app, bool start_enabled = true);
	virtual ~NPCManager();

	bool Start() override;
	update_status Update() override;
	update_status PostUpdate() override;
	bool CleanUp() override;

	// Access to NPC cars
	const std::vector<Car*>& GetNPCs() const { return npcCars; }
	Car* GetNPC(int index) const;

private:
	std::vector<Car*> npcCars;
	std::vector<PushAbility*> npcAbilities;

	void CreateNPC(const char* npcName, const char* texturePath);
	void UpdateAI(Car* npc);
	void CheckAndUseAbility(Car* npc, PushAbility* ability);
};