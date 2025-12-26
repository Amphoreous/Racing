#include "core/Globals.h"
#include "core/Application.h"
#include "modules/ModuleResources.h"
#include <algorithm>

ModuleResources::ModuleResources(Application* app, bool start_enabled) : Module(app, start_enabled)
{
}

ModuleResources::~ModuleResources()
{
}

bool ModuleResources::Init()
{
	LOG("Initializing Resource Manager");
	return true;
}

bool ModuleResources::CleanUp()
{
	LOG("Cleaning up Resource Manager");
	UnloadAll();
	return true;
}

std::string ModuleResources::NormalizePath(const char* path) const
{
	if (path == nullptr)
		return "";
	
	std::string normalized(path);
	// Convert backslashes to forward slashes for consistency
	std::replace(normalized.begin(), normalized.end(), '\\', '/');
	return normalized;
}

// Texture loading and management
Texture2D ModuleResources::LoadTexture(const char* path)
{
	if (path == nullptr)
	{
		LOG("ERROR: Attempted to load texture with null path");
		return Texture2D{ 0 };
	}

	std::string normalizedPath = NormalizePath(path);

	// Check if texture is already loaded
	if (textures.find(normalizedPath) != textures.end())
	{
		LOG("Texture already loaded: %s (Reference count: %d)", path, textureRefCount[normalizedPath]);
		textureRefCount[normalizedPath]++;
		return textures[normalizedPath];
	}

	// Load the texture from raylib
	LOG("Loading texture: %s", path);
	Texture2D texture = ::LoadTexture(path);

	if (texture.id == 0)
	{
		LOG("ERROR: Failed to load texture: %s", path);
		return Texture2D{ 0 };
	}

	// Store texture and initialize reference count
	textures[normalizedPath] = texture;
	textureRefCount[normalizedPath] = 1;

	LOG("Successfully loaded texture: %s (ID: %d)", path, texture.id);
	return texture;
}

void ModuleResources::UnloadTexture(const char* path)
{
	if (path == nullptr)
		return;

	std::string normalizedPath = NormalizePath(path);

	auto it = textures.find(normalizedPath);
	if (it == textures.end())
	{
		LOG("WARNING: Attempted to unload texture that was not loaded: %s", path);
		return;
	}

	// Decrease reference count
	if (textureRefCount[normalizedPath] > 1)
	{
		textureRefCount[normalizedPath]--;
		LOG("Texture reference count decreased: %s (Remaining: %d)", path, textureRefCount[normalizedPath]);
		return;
	}

	// Unload texture from raylib
	LOG("Unloading texture: %s (ID: %d)", path, it->second.id);
	::UnloadTexture(it->second);

	// Remove from maps
	textures.erase(it);
	textureRefCount.erase(normalizedPath);
}

bool ModuleResources::IsTextureLoaded(const char* path) const
{
	if (path == nullptr)
		return false;

	std::string normalizedPath = NormalizePath(path);
	return textures.find(normalizedPath) != textures.end();
}

void ModuleResources::UnloadAllTextures()
{
	LOG("Unloading all textures (%zu resources)", textures.size());
	for (auto& pair : textures)
	{
		LOG("Unloading texture: %s (ID: %d)", pair.first.c_str(), pair.second.id);
		::UnloadTexture(pair.second);
	}
	textures.clear();
	textureRefCount.clear();
}

int ModuleResources::GetTextureCount() const
{
	return (int)textures.size();
}

// Sound loading and management
Sound ModuleResources::LoadSound(const char* path)
{
	if (path == nullptr)
	{
		LOG("ERROR: Attempted to load sound with null path");
		return Sound{ 0 };
	}

	std::string normalizedPath = NormalizePath(path);

	// Check if sound is already loaded
	if (sounds.find(normalizedPath) != sounds.end())
	{
		LOG("Sound already loaded: %s (Reference count: %d)", path, soundRefCount[normalizedPath]);
		soundRefCount[normalizedPath]++;
		return sounds[normalizedPath];
	}

	// Load the sound from raylib
	LOG("Loading sound: %s", path);
	Sound sound = ::LoadSound(path);

	if (sound.stream.buffer == nullptr)
	{
		LOG("ERROR: Failed to load sound: %s", path);
		return Sound{ 0 };
	}

	// Store sound and initialize reference count
	sounds[normalizedPath] = sound;
	soundRefCount[normalizedPath] = 1;

	LOG("Successfully loaded sound: %s", path);
	return sound;
}

void ModuleResources::UnloadSound(const char* path)
{
	if (path == nullptr)
		return;

	std::string normalizedPath = NormalizePath(path);

	auto it = sounds.find(normalizedPath);
	if (it == sounds.end())
	{
		LOG("WARNING: Attempted to unload sound that was not loaded: %s", path);
		return;
	}

	// Decrease reference count
	if (soundRefCount[normalizedPath] > 1)
	{
		soundRefCount[normalizedPath]--;
		LOG("Sound reference count decreased: %s (Remaining: %d)", path, soundRefCount[normalizedPath]);
		return;
	}

	// Unload sound from raylib
	LOG("Unloading sound: %s", path);
	::UnloadSound(it->second);

	// Remove from maps
	sounds.erase(it);
	soundRefCount.erase(normalizedPath);
}

bool ModuleResources::IsSoundLoaded(const char* path) const
{
	if (path == nullptr)
		return false;

	std::string normalizedPath = NormalizePath(path);
	return sounds.find(normalizedPath) != sounds.end();
}

void ModuleResources::UnloadAllSounds()
{
	LOG("Unloading all sounds (%zu resources)", sounds.size());
	for (auto& pair : sounds)
	{
		LOG("Unloading sound: %s", pair.first.c_str());
		::UnloadSound(pair.second);
	}
	sounds.clear();
	soundRefCount.clear();
}

int ModuleResources::GetSoundCount() const
{
	return (int)sounds.size();
}

// Music loading and management
Music ModuleResources::LoadMusic(const char* path)
{
	if (path == nullptr)
	{
		LOG("ERROR: Attempted to load music with null path");
		return Music{ 0 };
	}

	std::string normalizedPath = NormalizePath(path);

	// Check if music is already loaded
	if (musics.find(normalizedPath) != musics.end())
	{
		LOG("Music already loaded: %s", path);
		return musics[normalizedPath];
	}

	// Load the music from raylib
	LOG("Loading music: %s", path);
	Music music = ::LoadMusicStream(path);

	if (!IsMusicValid(music))
	{
		LOG("ERROR: Failed to load music: %s", path);
		return Music{ 0 };
	}

	// Store music
	musics[normalizedPath] = music;

	LOG("Successfully loaded music: %s", path);
	return music;
}

void ModuleResources::UnloadMusic(const char* path)
{
	if (path == nullptr)
		return;

	std::string normalizedPath = NormalizePath(path);

	auto it = musics.find(normalizedPath);
	if (it == musics.end())
	{
		LOG("WARNING: Attempted to unload music that was not loaded: %s", path);
		return;
	}

	// Unload music from raylib
	LOG("Unloading music: %s", path);
	if (IsMusicValid(it->second))
	{
		StopMusicStream(it->second);
		::UnloadMusicStream(it->second);
	}

	// Remove from map
	musics.erase(it);
}

bool ModuleResources::IsMusicLoaded(const char* path) const
{
	if (path == nullptr)
		return false;

	std::string normalizedPath = NormalizePath(path);
	return musics.find(normalizedPath) != musics.end();
}

void ModuleResources::UnloadAllMusic()
{
	LOG("Unloading all music (%zu resources)", musics.size());
	for (auto& pair : musics)
	{
		LOG("Unloading music: %s", pair.first.c_str());
		if (IsMusicValid(pair.second))
		{
			StopMusicStream(pair.second);
			::UnloadMusicStream(pair.second);
		}
	}
	musics.clear();
}

int ModuleResources::GetMusicCount() const
{
	return (int)musics.size();
}

// Batch resource unloading
void ModuleResources::UnloadAll()
{
	UnloadAllTextures();
	UnloadAllSounds();
	UnloadAllMusic();
	LOG("All resources unloaded");
}

// Debug and statistics
void ModuleResources::PrintResourceReport() const
{
	LOG("Resource manager report:");
	LOG("Textures: %d loaded", GetTextureCount());
	for (const auto& pair : textures)
	{
		LOG("  - %s (ID: %d, Ref count: %d)", pair.first.c_str(), pair.second.id, textureRefCount.at(pair.first));
	}

	LOG("Sounds: %d loaded", GetSoundCount());
	for (const auto& pair : sounds)
	{
		LOG("  - %s (Ref count: %d)", pair.first.c_str(), soundRefCount.at(pair.first));
	}

	LOG("Music: %d loaded", GetMusicCount());
	for (const auto& pair : musics)
	{
		LOG("  - %s", pair.first.c_str());
	}

	int totalResources = GetTextureCount() + GetSoundCount() + GetMusicCount();
	LOG("Total Resources: %d", totalResources);
	LOG("============================================");
}

