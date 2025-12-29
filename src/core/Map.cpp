#include "core/Map.h"
#include "core/Application.h"
#include "core/Globals.h"
#include "modules/ModuleResources.h"
#include "modules/ModuleRender.h"
#include <sstream>
#include <fstream>
#include <algorithm>

Map::Map(Application* app, bool start_enabled) : Module(app, start_enabled), mapLoaded(false)
{
}

Map::~Map()
{
}

bool Map::Init()
{
    LOG("Initializing Map Module");
    return true;
}

bool Map::Start()
{
    LOG("Starting Map Module");
    bool ret = Load("assets/map/", "Map.tmx");
    return ret;
}

update_status Map::Update()
{
    // Map rendering happens in PostUpdate
    return UPDATE_CONTINUE;
}

update_status Map::PostUpdate()
{
    if (!mapLoaded)
        return UPDATE_CONTINUE;

    for (const auto& imageLayer : mapData.imageLayers)
    {
        if (imageLayer->texture.id != 0)
        {
            // Draw the entire image at its offset position
            App->renderer->Draw(imageLayer->texture,
                imageLayer->offsetX,
                imageLayer->offsetY,
                nullptr);  // nullptr = draw entire texture
        }
    }

    // Render all tile layers
    for (const auto& mapLayer : mapData.layers)
    {
        for (int i = 0; i < mapData.height; i++)
        {
            for (int j = 0; j < mapData.width; j++)
            {
                int gid = mapLayer->Get(i, j);
                if (gid == 0)
                    continue;

                TileSet* tileSet = GetTilesetFromTileId(gid);
                if (tileSet != nullptr && tileSet->texture.id != 0)
                {
                    Rectangle tileRect = tileSet->GetRect(gid);
                    vec2f mapCoord = MapToWorld(i, j);

                    App->renderer->Draw(tileSet->texture,
                        (int)mapCoord.x,
                        (int)mapCoord.y,
                        &tileRect);
                }
            }
        }
    }

    return UPDATE_CONTINUE;
}

TileSet* Map::GetTilesetFromTileId(int gid) const
{
    TileSet* set = nullptr;
    for (const auto& tileset : mapData.tilesets)
    {
        if (gid >= tileset->firstGid && gid < (tileset->firstGid + tileset->tileCount))
        {
            set = tileset;
            break;
        }
    }
    return set;
}

bool Map::CleanUp()
{
    LOG("Cleaning up Map Module");

    // Unload textures through resource manager
    for (const auto& tileset : mapData.tilesets)
    {
        if (!tileset->imagePath.empty())
        {
            App->resources->UnloadTexture(tileset->imagePath.c_str());
        }
        delete tileset;
    }
    mapData.tilesets.clear();

    for (const auto& layer : mapData.layers)
    {
        delete layer;
    }
    mapData.layers.clear();

    for (const auto& imageLayer : mapData.imageLayers)
    {
        if (!imageLayer->imagePath.empty())
        {
            App->resources->UnloadTexture(imageLayer->imagePath.c_str());
        }
        delete imageLayer;
    }
    mapData.imageLayers.clear();

    for (const auto& object : mapData.objects)
    {
        delete object;
    }
    mapData.objects.clear();

    mapLoaded = false;
    return true;
}

// Simple XML parser helper functions
static std::string GetAttributeValue(const std::string& line, const std::string& attribute)
{
    size_t pos = line.find(attribute + "=\"");
    if (pos == std::string::npos)
        return "";

    pos += attribute.length() + 2;
    size_t endPos = line.find("\"", pos);
    if (endPos == std::string::npos)
        return "";

    return line.substr(pos, endPos - pos);
}

static int GetAttributeInt(const std::string& line, const std::string& attribute, int defaultValue = 0)
{
    std::string value = GetAttributeValue(line, attribute);
    if (value.empty())
        return defaultValue;
    return std::stoi(value);
}

static std::string Trim(const std::string& str)
{
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

bool Map::Load(const std::string& path, const std::string& fileName)
{
    bool ret = false;
    mapFileName = fileName;
    mapPath = path;
    std::string fullPath = mapPath + mapFileName;

    LOG("Loading map: %s", fullPath.c_str());

    std::ifstream file(fullPath);
    if (!file.is_open())
    {
        LOG("ERROR: Could not open map file: %s", fullPath.c_str());
        return false;
    }

    std::string line;
    bool inLayer = false;
    bool inData = false;
    bool inTileset = false;
    bool inObjectGroup = false;
    bool inObject = false;
    bool inImageLayer = false;
    MapLayer* currentLayer = nullptr;
    TileSet* currentTileset = nullptr;
    MapObject* currentObject = nullptr;
    MapImageLayer* currentImageLayer = nullptr;
    std::string dataBuffer;

    while (std::getline(file, line))
    {
        line = Trim(line);

        // Parse map properties
        if (line.find("<map ") != std::string::npos)
        {
            mapData.width = GetAttributeInt(line, "width");
            mapData.height = GetAttributeInt(line, "height");
            mapData.tileWidth = GetAttributeInt(line, "tilewidth");
            mapData.tileHeight = GetAttributeInt(line, "tileheight");
            LOG("Map size: %dx%d, Tile size: %dx%d", mapData.width, mapData.height, mapData.tileWidth, mapData.tileHeight);
        }

        // Parse tileset
        if (line.find("<tileset ") != std::string::npos)
        {
            currentTileset = new TileSet();
            currentTileset->firstGid = GetAttributeInt(line, "firstgid");
            currentTileset->name = GetAttributeValue(line, "name");
            currentTileset->tileWidth = GetAttributeInt(line, "tilewidth");
            currentTileset->tileHeight = GetAttributeInt(line, "tileheight");
            currentTileset->spacing = GetAttributeInt(line, "spacing");
            currentTileset->margin = GetAttributeInt(line, "margin");
            currentTileset->tileCount = GetAttributeInt(line, "tilecount");
            currentTileset->columns = GetAttributeInt(line, "columns");
            inTileset = true;
        }

        if (inTileset && line.find("<image ") != std::string::npos)
        {
            std::string imageSource = GetAttributeValue(line, "source");
            if (!imageSource.empty() && currentTileset != nullptr)
            {
                currentTileset->imagePath = mapPath + imageSource;
                currentTileset->texture = App->resources->LoadTexture(currentTileset->imagePath.c_str());
                LOG("Loaded tileset texture: %s", currentTileset->imagePath.c_str());
            }
        }

        if (line.find("</tileset>") != std::string::npos)
        {
            if (currentTileset != nullptr)
            {
                mapData.tilesets.push_back(currentTileset);
                currentTileset = nullptr;
            }
            inTileset = false;
        }

        if (line.find("<imagelayer ") != std::string::npos)
        {
            currentImageLayer = new MapImageLayer();
            currentImageLayer->id = GetAttributeInt(line, "id");
            currentImageLayer->name = GetAttributeValue(line, "name");
            currentImageLayer->offsetX = GetAttributeInt(line, "offsetx", 0);
            currentImageLayer->offsetY = GetAttributeInt(line, "offsety", 0);
            inImageLayer = true;
            LOG("Loading image layer: %s", currentImageLayer->name.c_str());
        }

        if (inImageLayer && line.find("<image ") != std::string::npos)
        {
            std::string imageSource = GetAttributeValue(line, "source");
            if (!imageSource.empty() && currentImageLayer != nullptr)
            {
                currentImageLayer->imagePath = mapPath + imageSource;
                currentImageLayer->texture = App->resources->LoadTexture(currentImageLayer->imagePath.c_str());
                LOG("Loaded image layer texture: %s", currentImageLayer->imagePath.c_str());
            }
        }

        if (line.find("</imagelayer>") != std::string::npos)
        {
            if (currentImageLayer != nullptr)
            {
                mapData.imageLayers.push_back(currentImageLayer);
                currentImageLayer = nullptr;
            }
            inImageLayer = false;
        }

        // Parse layer
        if (line.find("<layer ") != std::string::npos)
        {
            currentLayer = new MapLayer();
            currentLayer->id = GetAttributeInt(line, "id");
            currentLayer->name = GetAttributeValue(line, "name");
            currentLayer->width = GetAttributeInt(line, "width");
            currentLayer->height = GetAttributeInt(line, "height");
            inLayer = true;
            LOG("Loading layer: %s (%dx%d)", currentLayer->name.c_str(), currentLayer->width, currentLayer->height);
        }

        if (inLayer && line.find("<data ") != std::string::npos)
        {
            inData = true;
            dataBuffer.clear();
        }

        if (inData)
        {
            if (line.find("</data>") != std::string::npos)
            {
                // Remove the closing tag
                size_t pos = line.find("</data>");
                if (pos != std::string::npos)
                {
                    dataBuffer += line.substr(0, pos);
                }

                // Parse CSV data
                if (currentLayer != nullptr)
                {
                    std::stringstream ss(dataBuffer);
                    std::string token;
                    while (std::getline(ss, token, ','))
                    {
                        token = Trim(token);
                        if (!token.empty())
                        {
                            currentLayer->tiles.push_back(std::stoi(token));
                        }
                    }
                    LOG("Layer '%s' loaded with %d tiles", currentLayer->name.c_str(), (int)currentLayer->tiles.size());
                }

                inData = false;
            }
            else if (line.find("<data") == std::string::npos)
            {
                dataBuffer += line;
            }
        }

        if (line.find("</layer>") != std::string::npos)
        {
            if (currentLayer != nullptr)
            {
                mapData.layers.push_back(currentLayer);
                currentLayer = nullptr;
            }
            inLayer = false;
        }

        // Parse object groups
        if (line.find("<objectgroup ") != std::string::npos)
        {
            inObjectGroup = true;
        }

        if (inObjectGroup && line.find("<object ") != std::string::npos)
        {
            currentObject = new MapObject();
            currentObject->id = GetAttributeInt(line, "id");
            currentObject->name = GetAttributeValue(line, "name");
            currentObject->type = GetAttributeValue(line, "type");
            currentObject->x = GetAttributeInt(line, "x");
            currentObject->y = GetAttributeInt(line, "y");
            currentObject->width = GetAttributeInt(line, "width");
            currentObject->height = GetAttributeInt(line, "height");

            // Check if it's a self-closing tag
            if (line.find("/>") != std::string::npos)
            {
                mapData.objects.push_back(currentObject);
                LOG("Loaded object: %s at (%d, %d)", currentObject->name.c_str(), currentObject->x, currentObject->y);
                currentObject = nullptr;
            }
            else
            {
                inObject = true;
            }
        }

        if (line.find("</object>") != std::string::npos)
        {
            if (currentObject != nullptr)
            {
                mapData.objects.push_back(currentObject);
                LOG("Loaded object: %s at (%d, %d)", currentObject->name.c_str(), currentObject->x, currentObject->y);
                currentObject = nullptr;
            }
            inObject = false;
        }

        if (line.find("</objectgroup>") != std::string::npos)
        {
            inObjectGroup = false;
        }
    }

    file.close();

    ret = !mapData.layers.empty() || !mapData.imageLayers.empty() || !mapData.objects.empty();
    mapLoaded = ret;

    if (ret)
    {
        LOG("Successfully loaded map: %s", fileName.c_str());
        LOG("Tilesets: %d, Layers: %d, Image Layers: %d, Objects: %d",
            (int)mapData.tilesets.size(),
            (int)mapData.layers.size(),
            (int)mapData.imageLayers.size(),
            (int)mapData.objects.size());
    }
    else
    {
        LOG("ERROR: Failed to load map data from: %s", fileName.c_str());
    }

    return ret;
}

vec2f Map::MapToWorld(int i, int j) const
{
    vec2f ret;
    ret.x = (float)(j * mapData.tileWidth);
    ret.y = (float)(i * mapData.tileHeight);
    return ret;
}

vec2i Map::WorldToMap(int x, int y) const
{
    vec2i ret;
    ret.x = x / mapData.tileWidth;
    ret.y = y / mapData.tileHeight;
    return ret;
}

MapObject* Map::GetObjectByName(const std::string& name) const
{
    for (const auto& object : mapData.objects)
    {
        if (object->name == name)
        {
            return object;
        }
    }
    LOG("Warning: Object '%s' not found in map.", name.c_str());
    return nullptr;
}