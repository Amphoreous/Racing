#include "Globals.h"
#include "Application.h"
#include "ModuleAudio.h"
#include "ModuleResources.h"

#include "raylib.h"

#define MAX_FX_SOUNDS   64

ModuleAudio::ModuleAudio(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	fx_count = 0;
	music = Music{ 0 };
}

ModuleAudio::~ModuleAudio()
{}

bool ModuleAudio::Init()
{
	LOG("Initializing audio system");
    InitAudioDevice();
	return true;
}

bool ModuleAudio::CleanUp()
{
	LOG("Shutting down audio");
    // Resources are cleaned up by ModuleResources automatically
    CloseAudioDevice();
	return true;
}

// Play music using the resource manager
bool ModuleAudio::PlayMusic(const char* path, float fade_time)
{
	if(IsEnabled() == false)
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
    
    PlayMusicStream(music);

	LOG("Successfully playing %s", path);

	return ret;
}

// Load WAV - now uses the resource manager
unsigned int ModuleAudio::LoadFx(const char* path)
{
	if(IsEnabled() == false)
		return 0;

	unsigned int ret = 0;

	// Load sound through resource manager
	Sound sound = App->resources->LoadSound(path);

	if(sound.stream.buffer == NULL)
	{
		LOG("Cannot load sound: %s", path);
	}
	else
	{
        fx[fx_count++] = sound;
		ret = fx_count;
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

	if(id < fx_count) PlaySound(fx[id]);

	return ret;
}