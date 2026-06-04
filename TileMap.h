#pragma once
#include<vector>
#include<string>
#include<SDL3/SDL.h>
#include "json.hpp"
#include<SDL3_image/SDL_image.h>
#include<SDL3/SDL_log.h>
#include <fstream>  
class TileMap {
public:
    int tileWidth = 64;
    int tileHeight = 64;
    int mapWidth = 0, mapHeight = 0;

    // Для каждого слоя храним одномерный массив тайлов
    std::vector<std::vector<int>> layerTiles; // [layerIndex][tileIndex]

    // Текстурные данные
    SDL_Texture* tilesetTex = nullptr;
    int firstgid = 1;
    int texWidth = 0, texHeight = 0;
    int texTileWidth = 0, texTileHeight = 0;

    // Загрузка всех слоёв из JSON
    bool loadFromJSON(const std::string& path);
    // Загрузка текстуры тайлсета (без изменений)
    bool loadTilesetTexture(SDL_Renderer* renderer, const std::string& imagePath,
        int tw = 0, int th = 0);

    // Отрисовка всех слоёв подряд
    void render(SDL_Renderer* renderer, float camX, float camY, float zoom) const;
    ~TileMap();
};
