#pragma once

#include "core/Module.h"
#include "core/p2Point.h"
#include "raylib.h"
#include <list>
#include <vector>
#include <string>

struct Properties
{
    struct Property
    {
        std::string name;
        std::string value;
    };

    std::list<Property*> propertyList;

    ~Properties()
    {
        for (const auto& property : propertyList)
        {
            delete property;
        }
        propertyList.clear();
    }

    Property* GetProperty(const char* name)
    {
        for (const auto& property : propertyList) {
            if (property->name == name) {
                return property;
            }
        }
        return nullptr;
    }
};

struct MapLayer
{
    int id;
    std::string name;
    int width;
    int height;
    std::vector<int> tiles;
    Properties properties;

    unsigned int Get(int i, int j) const
    {
        if (i < 0 || i >= height || j < 0 || j >= width)
            return 0;
        return tiles[(i * width) + j];
    }
};

struct MapImageLayer
{
    int id;
    std::string name;
    std::string imagePath;
    Texture2D texture;
    int offsetX;
    int offsetY;
    Properties properties;
};

struct TileSet
{
    int firstGid;
    std::string name;
    int tileWidth;
    int tileHeight;
    int spacing;
    int margin;
    int tileCount;
    int columns;
    std::string imagePath;
    Texture2D texture;

    Rectangle GetRect(unsigned int gid) {
        Rectangle rect = { 0 };

        int relativeIndex = gid - firstGid;
        rect.width = (float)tileWidth;
        rect.height = (float)tileHeight;
        rect.x = (float)(margin + (tileWidth + spacing) * (relativeIndex % columns));
        rect.y = (float)(margin + (tileHeight + spacing) * (relativeIndex / columns));

        return rect;
    }
};

struct MapObject
{
    int id;
    std::string name;
    std::string type;
    int x, y, width, height;
    Properties properties;
    
    // Polygon data
    std::vector<vec2i> polygonPoints;
    bool hasPolygon = false;
};

struct MapData
{
    int width;
    int height;
    int tileWidth;
    int tileHeight;
    std::list<TileSet*> tilesets;
    std::list<MapLayer*> layers;
    std::list<MapImageLayer*> imageLayers;
    std::list<MapObject*> objects;
};

class Map : public Module
{
public:
    Map(Application* app, bool start_enabled = true);
    virtual ~Map();

    bool Init() override;
    bool Start() override;
    update_status Update() override;
    update_status PostUpdate() override;
    bool CleanUp() override;

    bool Load(const std::string& path, const std::string& mapFileName);
    vec2f MapToWorld(int i, int j) const;
    vec2i WorldToMap(int x, int y) const;
    TileSet* GetTilesetFromTileId(int gid) const;
    MapObject* GetObjectByName(const std::string& name) const;
    void RenderMap() const;

public:
    std::string mapFileName;
    std::string mapPath;
    MapData mapData;

private:
    bool mapLoaded;
    void CreateCollisionBodies();
    bool TriangulatePolygon(const std::vector<vec2i>& polygon, std::vector<std::vector<float>>& triangles);
    bool IsEar(const std::vector<vec2i>& vertices, size_t prev, size_t current, size_t next);
    float CrossProduct(vec2i v1, vec2i v2);
    bool PointInTriangle(vec2i p, vec2i a, vec2i b, vec2i c);
};