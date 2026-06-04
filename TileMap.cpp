#include "TileMap.h"
using json = nlohmann::json;

bool TileMap::loadFromJSON(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        SDL_Log("Не могу открыть JSON: %s", path.c_str());
        return false;
    }
    json j;
    file >> j;

    if (!j.contains("width") || !j.contains("height") ||
        !j.contains("tilewidth") || !j.contains("tileheight")) {
        SDL_Log("JSON не содержит width/height/tilewidth/tileheight");
        return false;
    }

    mapWidth = j["width"];
    mapHeight = j["height"];
    tileWidth = j["tilewidth"];
    tileHeight = j["tileheight"];

    if (j.contains("tilesets") && j["tilesets"].is_array() && !j["tilesets"].empty()) {
        firstgid = j["tilesets"][0].value("firstgid", 1);
    }
    texTileWidth = tileWidth;
    texTileHeight = tileHeight;

    if (!j.contains("layers") || !j["layers"].is_array()) {
        SDL_Log("Нет массива layers");
        return false;
    }

    layerTiles.clear();
    size_t expected = (size_t)mapWidth * mapHeight;

    // Проходим по всем слоям
    for (auto& layer : j["layers"]) {
        if (!layer.contains("data") || !layer["data"].is_array()) {
            // не тайловый слой – пропускаем
            continue;
        }
        auto data = layer["data"];
        std::vector<int> tiles(expected, 0);
        for (size_t i = 0; i < expected && i < data.size(); ++i) {
            tiles[i] = data[i].get<int>();
        }
        layerTiles.push_back(std::move(tiles));
    }

    SDL_Log("Загружено слоёв: %d (размер %dx%d)", (int)layerTiles.size(), mapWidth, mapHeight);
    return true;
}

// Загрузка текстуры тайлсета (без изменений)
bool TileMap::loadTilesetTexture(SDL_Renderer* renderer, const std::string& imagePath, int tw, int th) {
    if (tilesetTex) {
        SDL_DestroyTexture(tilesetTex);
        tilesetTex = nullptr;
    }

    tilesetTex = IMG_LoadTexture(renderer, imagePath.c_str());
    if (!tilesetTex) {
        SDL_Log("Не загрузилась текстура тайлсета: %s", SDL_GetError());
        return false;
    }

    float fw, fh;
    SDL_GetTextureSize(tilesetTex, &fw, &fh);
    texWidth = (int)fw;
    texHeight = (int)fh;

    texTileWidth = (tw > 0) ? tw : tileWidth;
    texTileHeight = (th > 0) ? th : tileHeight;

    SDL_Log("Тайлсет загружен: %dx%d, тайл %dx%d",
        texWidth, texHeight, texTileWidth, texTileHeight);
    return true;
}

// Отрисовка всех слоёв подряд
void TileMap::render(SDL_Renderer* renderer, float camX, float camY, float zoom) const {
    if (!tilesetTex) return;

    int cols = texWidth / texTileWidth;

    // Границы видимости
    float viewW = 1920.0f / zoom;
    float viewH = 1080.0f / zoom;
    int startCol = (int)(camX / tileWidth) - 1;
    int startRow = (int)(camY / tileHeight) - 1;
    int endCol = (int)((camX + viewW) / tileWidth) + 1;
    int endRow = (int)((camY + viewH) / tileHeight) + 1;
    if (startCol < 0) startCol = 0;
    if (startRow < 0) startRow = 0;
    if (endCol > mapWidth) endCol = mapWidth;
    if (endRow > mapHeight) endRow = mapHeight;

    // Рисуем все слои по очереди
    for (const auto& tiles : layerTiles) {
        for (int row = startRow; row < endRow; ++row) {
            for (int col = startCol; col < endCol; ++col) {
                int index = row * mapWidth + col;
                if (index >= (int)tiles.size()) continue;
                int gid = tiles[index];
                if (gid == 0) continue;

                int localID = gid - firstgid;
                if (localID < 0) continue;
                // Проверка на выход за границы тайлсета
                if (localID >= cols * (texHeight / texTileHeight)) continue;

                int srcX = (localID % cols) * texTileWidth;
                int srcY = (localID / cols) * texTileHeight;

                SDL_FRect srcRect = { (float)srcX, (float)srcY,
                                      (float)texTileWidth, (float)texTileHeight };
                float screenX = (col * tileWidth - camX) * zoom;
                float screenY = (row * tileHeight - camY) * zoom;
                SDL_FRect dstRect = { screenX, screenY,
                                      tileWidth * zoom, tileHeight * zoom };

                SDL_RenderTexture(renderer, tilesetTex, &srcRect, &dstRect);
            }
        }
    }
}
TileMap::~TileMap() {
    if (tilesetTex) {
        SDL_DestroyTexture(tilesetTex);
        tilesetTex = nullptr;
    }
}