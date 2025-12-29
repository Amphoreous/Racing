#pragma once
#include "core/Module.h"
#include "core/Globals.h"

#include <limits.h>

// Camera view modes for different viewing options
enum CameraViewMode
{
	CAMERA_FOLLOW_CAR,           // Camera follows the car with rotation
	CAMERA_FOLLOW_CAR_NO_ROT,    // Camera follows the car without rotation
	CAMERA_FULL_MAP              // Camera shows the full map without rotation
};

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
	void HandleCameraInput();

public:
	Color background;
	Camera2D camera;
	CameraViewMode cameraMode;
};