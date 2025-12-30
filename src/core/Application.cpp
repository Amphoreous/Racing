#include "core/Module.h"
#include "modules/ModuleWindow.h"
#include "modules/ModuleRender.h"
#include "modules/ModuleAudio.h"
#include "modules/ModulePhysics.h"
#include "modules/ModuleGame.h"
#include "modules/ModuleResources.h"
#include "entities/Player.h"
#include "entities/NPCManager.h"
#include "core/Map.h"
#include "entities/CheckpointManager.h"
#include "modules/ModuleMainMenu.h"

#include "core/Application.h"

Application::Application()
{
	window = new ModuleWindow(this);
	resources = new ModuleResources(this, true);
	map = new Map(this, true);
	scene_intro = new ModuleGame(this);
	audio = new ModuleAudio(this, true);
	physics = new ModulePhysics(this);
	player = new ModulePlayer(this);
	npcManager = new NPCManager(this);  // AÑADIR ESTA LÍNEA
	checkpointManager = new CheckpointManager(this);
	mainMenu = new ModuleMainMenu(this);
	renderer = new ModuleRender(this);

	// Module initialization order matters - resources first, rendering last
	AddModule(window);
	AddModule(resources);  // Load resources early
	AddModule(scene_intro); // Game scene (background)
	AddModule(audio);
	AddModule(map);
	AddModule(player);     // Player car
	AddModule(npcManager); // AÑADIR ESTA LÍNEA - NPC cars after player
	AddModule(checkpointManager);
	AddModule(mainMenu);
	AddModule(physics);    // Physics debug render - on top of car
	AddModule(renderer);

	// Disable game modules initially, enable them from menu
	scene_intro->Disable();
	physics->Disable();
	player->Disable();
	npcManager->Disable();
	map->Disable();
	checkpointManager->Disable();
}

Application::~Application()
{
	for (auto it = list_modules.rbegin(); it != list_modules.rend(); ++it)
	{
		Module* item = *it;
		delete item;
	}
	list_modules.clear();
}

bool Application::Init()
{
	bool ret = true;

	// Call Init() in all modules
	for (auto it = list_modules.begin(); it != list_modules.end() && ret; ++it)
	{
		Module* module = *it;
		ret = module->Init();
	}

	// After all Init calls we call Start() in all modules that are ENABLED
	LOG("Application Start --------------");

	for (auto it = list_modules.begin(); it != list_modules.end() && ret; ++it)
	{
		Module* module = *it;
		// Only call Start() on enabled modules
		// Disabled modules will have Start() called when Enable() is invoked
		if (module->IsEnabled())
		{
			ret = module->Start();
		}
	}

	return ret;
}

// Call PreUpdate, Update and PostUpdate on all modules
update_status Application::Update()
{
	update_status ret = UPDATE_CONTINUE;

	if (state == GAME_MENU)
	{
		// Only update menu and renderer for drawing
		ret = renderer->PreUpdate();
		if (ret == UPDATE_CONTINUE)
		{
			ret = mainMenu->Update();
		}
		if (ret == UPDATE_CONTINUE)
		{
			ret = mainMenu->PostUpdate();
		}
		if (ret == UPDATE_CONTINUE)
		{
			ret = renderer->PostUpdate();
		}
	}
	else
	{
		for (auto it = list_modules.begin(); it != list_modules.end() && ret == UPDATE_CONTINUE; ++it)
		{
			Module* module = *it;
			if (module->IsEnabled())
			{
				ret = module->PreUpdate();
			}
		}

		for (auto it = list_modules.begin(); it != list_modules.end() && ret == UPDATE_CONTINUE; ++it)
		{
			Module* module = *it;
			if (module->IsEnabled())
			{
				ret = module->Update();
			}
		}

		for (auto it = list_modules.begin(); it != list_modules.end() && ret == UPDATE_CONTINUE; ++it)
		{
			Module* module = *it;
			if (module->IsEnabled())
			{
				ret = module->PostUpdate();
			}
		}
	}

	if (WindowShouldClose()) ret = UPDATE_STOP;

	return ret;
}

bool Application::CleanUp()
{
	bool ret = true;
	for (auto it = list_modules.rbegin(); it != list_modules.rend() && ret; ++it)
	{
		Module* item = *it;
		ret = item->CleanUp();
	}

	return ret;
}

void Application::AddModule(Module* mod)
{
	list_modules.emplace_back(mod);
}