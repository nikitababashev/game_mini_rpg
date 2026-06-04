#include "NPC1.h"
#include <SDL3/SDL_log.h>   // для SDL_Log

NPC1::NPC1(SDL_Renderer* renderer, const std::string& texPath, float x, float y)
    : renderer(renderer), worldX(x), worldY(y)
{
    texture = IMG_LoadTexture(renderer, texPath.c_str());
    if (!texture) SDL_Log("NPC1 texture error: %s", SDL_GetError());
}

NPC1::~NPC1() {
    if (texture) SDL_DestroyTexture(texture);
}

void NPC1::draw(float camX, float camY, float zoom) {
    float drawX = (worldX - width / 2.0f - camX) * zoom;
    float drawY = (worldY - height / 2.0f - camY) * zoom;
    SDL_FRect dest = { drawX, drawY, width * zoom, height * zoom };
    SDL_FRect src = { 0, 0, 64, 64 };
    SDL_RenderTexture(renderer, texture, &src, &dest);
}

bool NPC1::isPlayerNear(float playerX, float playerY, float dist) {   // убрано = 60.0f
    float dx = worldX - playerX;
    float dy = worldY - playerY;
    return (dx * dx + dy * dy) <= (dist * dist);
}