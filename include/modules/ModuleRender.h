#pragma once
#include "core/Module.h"
#include "core/Globals.h"

#include <limits.h>

// ModuleRender: Handles all drawing operations
// NOTE: All textures should be loaded via ModuleResources (App->resources->LoadTexture())
// This module only handles drawing, NOT resource management
class ModuleRender : public Module
{
public:
	ModuleRender(Application* app, bool start_enabled = true);
	~ModuleRender();

	bool Init();
	bool Start();
	update_status PreUpdate();
	update_status Update();
	update_status PostUpdate();
	bool CleanUp();

	void SetBackgroundColor(Color color);
	bool Draw(Texture2D texture, int x, int y, const Rectangle* section = NULL, double angle = 0, int pivot_x = 0, int pivot_y = 0) const;
	bool DrawText(const char* text, int x, int y, Font font, int spacing, Color tint) const;

private:
	void UpdateCamera();

public:
	Color background;
	Camera2D camera;
};