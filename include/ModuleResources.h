#pragma once

#include "Module.h"
#include "Globals.h"
#include "raylib.h"
#include <map>
#include <string>

class ModuleResources : public Module
{
public:
	ModuleResources(Application* app, bool start_enabled = true);
	~ModuleResources();

	bool Init();
	bool CleanUp();

	// Texture resource management
	Texture2D LoadTexture(const char* path);
	void UnloadTexture(const char* path);
	bool IsTextureLoaded(const char* path) const;

	// Sound resource management
	Sound LoadSound(const char* path);
	void UnloadSound(const char* path);
	bool IsSoundLoaded(const char* path) const;

	// Music resource management
	Music LoadMusic(const char* path);
	void UnloadMusic(const char* path);
	bool IsMusicLoaded(const char* path) const;

	// Resource cleanup and statistics
	void UnloadAllTextures();
	void UnloadAllSounds();
	void UnloadAllMusic();
	void UnloadAll();

	int GetTextureCount() const;
	int GetSoundCount() const;
	int GetMusicCount() const;

	// Debug and statistics
	void PrintResourceReport() const;

private:
	// Resource storage maps
	std::map<std::string, Texture2D> textures;
	std::map<std::string, Sound> sounds;
	std::map<std::string, Music> musics;
	std::map<std::string, int> textureRefCount;
	std::map<std::string, int> soundRefCount;

	// Helper methods
	std::string NormalizePath(const char* path) const;
};
