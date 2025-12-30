#include "core/Map.h"
#include "core/Application.h"
#include "core/Globals.h"
#include "modules/ModuleResources.h"
#include "modules/ModuleRender.h"
#include "modules/ModulePhysics.h"
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
    
    // If map is already loaded, skip re-loading to prevent duplicates
    // The map data persists through Enable/Disable cycles once loaded
    if (mapLoaded)
    {
        LOG("Map already loaded - skipping reload");
        return true;
    }
    
    bool ret = Load("assets/map/", "Map.tmx");

    if (ret)
    {
        // Create collision bodies for map objects
        CreateCollisionBodies();
    }

    return ret;
}

update_status Map::Update()
{
    // Map rendering happens in PostUpdate
    return UPDATE_CONTINUE;
}

update_status Map::PostUpdate()
{
    // Map rendering is now handled by ModuleRender within camera space
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

// Create physics bodies for collision objects
bool Map::TriangulatePolygon(const std::vector<vec2i>& polygon, std::vector<std::vector<float>>& triangles)
{
	if (polygon.size() < 3)
		return false;

	// Use ear clipping algorithm for proper triangulation of concave polygons
	std::vector<vec2i> vertices = polygon;
	triangles.clear();

	while (vertices.size() > 3)
	{
		bool foundEar = false;
		
		for (size_t i = 0; i < vertices.size(); ++i)
		{
			size_t prev = (i + vertices.size() - 1) % vertices.size();
			size_t next = (i + 1) % vertices.size();
			
			// Check if this vertex forms an ear
			if (IsEar(vertices, prev, i, next))
			{
				// Create triangle from prev, current, next
				std::vector<float> triangle;
				triangle.push_back(vertices[prev].x);
				triangle.push_back(vertices[prev].y);
				triangle.push_back(vertices[i].x);
				triangle.push_back(vertices[i].y);
				triangle.push_back(vertices[next].x);
				triangle.push_back(vertices[next].y);
				
				triangles.push_back(triangle);
				
				// Remove the ear vertex
				vertices.erase(vertices.begin() + i);
				foundEar = true;
				break;
			}
		}
		
		if (!foundEar)
		{
			// If no ear found, the polygon might be degenerate
			// Fall back to a simple approach
			break;
		}
	}
	
	// Add the final triangle
	if (vertices.size() == 3)
	{
		std::vector<float> triangle;
		triangle.push_back(vertices[0].x);
		triangle.push_back(vertices[0].y);
		triangle.push_back(vertices[1].x);
		triangle.push_back(vertices[1].y);
		triangle.push_back(vertices[2].x);
		triangle.push_back(vertices[2].y);
		
		triangles.push_back(triangle);
	}
	
	return !triangles.empty();
}

bool Map::IsEar(const std::vector<vec2i>& vertices, size_t prev, size_t current, size_t next)
{
	// Check if the triangle (prev, current, next) contains any other vertices
	vec2i a = vertices[prev];
	vec2i b = vertices[current];
	vec2i c = vertices[next];
	
	// Check if triangle is counter-clockwise (convex)
	if (CrossProduct(b - a, c - b) <= 0)
		return false;
	
	// Check if any other vertex is inside this triangle
	for (size_t i = 0; i < vertices.size(); ++i)
	{
		if (i == prev || i == current || i == next)
			continue;
		
		if (PointInTriangle(vertices[i], a, b, c))
			return false;
	}
	
	return true;
}

float Map::CrossProduct(vec2i v1, vec2i v2)
{
	return v1.x * v2.y - v1.y * v2.x;
}

bool Map::PointInTriangle(vec2i p, vec2i a, vec2i b, vec2i c)
{
	// Use barycentric coordinate system to check if point is inside triangle
	float area = CrossProduct(b - a, c - a);
	float area1 = CrossProduct(p - a, b - a);
	float area2 = CrossProduct(p - b, c - b);
	float area3 = CrossProduct(p - c, a - c);
	
	// Check if all areas have the same sign as the total area
	return (area1 >= 0 && area2 >= 0 && area3 >= 0 && area > 0) ||
		   (area1 <= 0 && area2 <= 0 && area3 <= 0 && area < 0);
}

void Map::CreateCollisionBodies()
{
    if (!App->physics)
    {
        LOG("Warning: Physics module not available for collision creation");
        return;
    }

    // Check for tile layer named "Collisions" and create collision bodies for each tile
    for (const auto& layer : mapData.layers)
    {
        // Only process the layer that is exactly named "Collisions"
        if (layer->name == "Collisions")
        {
            for (int y = 0; y < mapData.height; ++y)
            {
                for (int x = 0; x < mapData.width; ++x)
                {
                    int gid = layer->Get(y, x);

                    // If the tile is not empty (gid != 0), create collision
                    if (gid != 0)
                    {
                        // Calculate world position (x, y are grid indices)
                        float tileX = x * mapData.tileWidth;
                        float tileY = y * mapData.tileHeight;

                        // Box2D usually wants the center of the rectangle, so add half the size
                        float centerX = tileX + (mapData.tileWidth / 2.0f);
                        float centerY = tileY + (mapData.tileHeight / 2.0f);

                        // Create static body
                        PhysBody* body = App->physics->CreateRectangle(centerX, centerY, mapData.tileWidth, mapData.tileHeight, PhysBody::BodyType::STATIC);
                        if (body)
                        {
                            LOG("Created tile collision body at (%.1f, %.1f) size (%.1f, %.1f)",
                                centerX, centerY, (float)mapData.tileWidth, (float)mapData.tileHeight);
                        }
                        else
                        {
                            LOG("Failed to create tile collision body at (%d, %d)", x, y);
                        }
                    }
                }
            }
        }
    }

    // Process map objects
    for (const auto& object : mapData.objects)
    {
        // 1. Skip logic objects
        if (object->type == "Start" || object->name.find("C") == 0 || object->name == "FL")
        {
            continue;
        }

        // 2. Skip zones (The car detects them by code, they should NOT create solid physics)
        // If we create physics for them, the car will collide against water/mud like a solid wall.
        if (object->type == "Normal" || object->type == "Water" || object->type == "Mud")
        {
            // LOG("Zone object '%s' (%s) skipped for physics creation (handled by logic)", object->name.c_str(), object->type.c_str());
            continue; 
        }

        // 3. Create walls (Only Polylines/Chains that are not zones)
        if (object->hasPolygon && !object->polygonPoints.empty())
        {
            std::vector<float> worldVertices;
            for (const auto& point : object->polygonPoints)
            {
                worldVertices.push_back((float)object->x + point.x);
                worldVertices.push_back((float)object->y + point.y);
            }

            // Create the physical chain (Polylines have isClosed = false -> Will use the two-sided edges fix)
            PhysBody* body = App->physics->CreateChain(0, 0, worldVertices.data(), worldVertices.size() / 2, object->isClosed);

            if (body)
            {
                body->SetUserData((void*)object);
                LOG("Created Wall/Collision body for object '%s' (Points: %d)", object->name.c_str(), (int)object->polygonPoints.size());
            }
        }
        else if (object->width > 0 && object->height > 0)
        {
            // Create rectangle collision body for objects without polygons
            // Position is center of rectangle
            float centerX = object->x + object->width * 0.5f;
            float centerY = object->y + object->height * 0.5f;

            PhysBody* body = App->physics->CreateRectangle(centerX, centerY, object->width, object->height, PhysBody::BodyType::STATIC);
            if (body)
            {
                // Store reference to the map object for terrain type detection
                body->SetUserData((void*)object);

                LOG("Created rectangle collision body for object '%s' at (%.1f, %.1f) size (%.1f, %.1f)",
                    object->name.c_str(), centerX, centerY, object->width, object->height);
            }
            else
            {
                LOG("Failed to create rectangle collision body for object '%s'", object->name.c_str());
            }
        }
    }
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
	bool inProperties = false;
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

		// Parse object properties
		if (inObject && line.find("<properties>") != std::string::npos)
		{
			inProperties = true;
		}

		if (inProperties && line.find("<property ") != std::string::npos)
		{
			if (currentObject)
			{
				Properties::Property* prop = new Properties::Property();
				prop->name = GetAttributeValue(line, "name");
				prop->value = GetAttributeValue(line, "value");
				currentObject->properties.propertyList.push_back(prop);
				LOG("  Property: %s = %s", prop->name.c_str(), prop->value.c_str());
			}
		}

		if (inProperties && line.find("</properties>") != std::string::npos)
		{
			inProperties = false;
		}

// Parse polygon or polyline data
        bool isPolygon = (line.find("<polygon ") != std::string::npos);
        bool isPolyline = (line.find("<polyline ") != std::string::npos);

        if (inObject && (isPolygon || isPolyline))
        {
            if (currentObject)
            {
                std::string pointsStr = GetAttributeValue(line, "points");
                if (!pointsStr.empty())
                {
                    currentObject->hasPolygon = true;
                    // If it's a polygon it's closed, if it's a polyline it's open
                    currentObject->isClosed = isPolygon;

                    std::stringstream ss(pointsStr);
                    std::string pointStr;

                    while (std::getline(ss, pointStr, ' '))
                    {
                        pointStr = Trim(pointStr);
                        if (!pointStr.empty())
                        {
                            size_t commaPos = pointStr.find(',');
                            if (commaPos != std::string::npos)
                            {
                                int px = std::stoi(pointStr.substr(0, commaPos));
                                int py = std::stoi(pointStr.substr(commaPos + 1));
                                currentObject->polygonPoints.push_back({px, py});
                            }
                        }
                    }
                    LOG("  Shape with %d points (Closed: %s)",
                        (int)currentObject->polygonPoints.size(), currentObject->isClosed ? "Yes" : "No");
				}
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

void Map::RenderMap() const
{
    if (!mapLoaded)
        return;

    // Render all image layers
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
        // If it's the collision layer, don't draw it
        if (mapLayer->name == "Collisions")
        {
            continue;
        }

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
}