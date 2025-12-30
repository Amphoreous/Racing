#include "core/Globals.h"
#include "core/Application.h"
#include "modules/ModuleAudio.h"
#include "modules/ModuleResources.h"

#include "raylib.h"

#define MAX_FX_SOUNDS   64

ModuleAudio::ModuleAudio(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	fx_count = 0;
	music = Music{ 0 };
}

ModuleAudio::~ModuleAudio()
{
}

bool ModuleAudio::Init()
{
	LOG("Initializing audio system");
	InitAudioDevice();

	if (!IsAudioDeviceReady())
	{
		LOG("ERROR: Audio device failed to initialize!");
		return false;
	}

	LOG("Audio device initialized successfully");
	return true;
}

update_status ModuleAudio::Update()
{
	// CRITICAL: Update music stream every frame (required for music playback)
	if (IsMusicValid(music))
	{
		UpdateMusicStream(music);
	}

	return UPDATE_CONTINUE;
}

bool ModuleAudio::CleanUp()
{
	LOG("Shutting down audio");

	// Stop and unload music if playing
	if (IsMusicValid(music))
	{
		StopMusicStream(music);
	}

	// Resources are cleaned up by ModuleResources automatically
	CloseAudioDevice();
	return true;
}

// Play music using the resource manager
bool ModuleAudio::PlayMusic(const char* path, float fade_time)
{
	if (IsEnabled() == false)
		return false;

	bool ret = true;

	// Stop current music if playing
	if (IsMusicValid(music))
	{
		StopMusicStream(music);
	}

	// Load music through resource manager
	music = App->resources->LoadMusic(path);

	if (!IsMusicValid(music))
	{
		LOG("ERROR: Could not load music: %s", path);
		return false;
	}

	// CRITICAL: Enable looping for the music
	music.looping = true;

	PlayMusicStream(music);

	// Set volume for background music (lower volume so it doesn't overpower gameplay)
	SetMusicVolume(music, 0.05f);

	LOG("Successfully playing %s (looping enabled, background volume: 0.2)", path);

	return ret;
}

// Load WAV - now uses the resource manager
unsigned int ModuleAudio::LoadFx(const char* path)
{
	if (IsEnabled() == false)
		return 0;

	unsigned int ret = 0;

	// Load sound through resource manager
	Sound sound = App->resources->LoadSound(path);

	if (sound.stream.buffer == NULL)
	{
		LOG("Cannot load sound: %s", path);
	}
	else
	{
		fx[fx_count] = sound;        // Assign to current index
		ret = fx_count + 1;          // Return ID (1-indexed)
		fx_count++;                  // Increment counter
	}

	return ret;
}

// Play WAV
bool ModuleAudio::PlayFx(unsigned int id, int repeat)
{
	if (IsEnabled() == false)
	{
		return false;
	}

	bool ret = false;

	// ID is 1-indexed, convert to 0-indexed array access
	if (id > 0 && id <= fx_count)
	{
		PlaySound(fx[id - 1]);  // Convert 1-indexed ID to 0-indexed array
		ret = true;
	}

	return ret;
}