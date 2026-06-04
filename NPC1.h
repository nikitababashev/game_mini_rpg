#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>

class NPC1 {
public:
    float worldX, worldY;
    float width = 64, height = 64;
    SDL_Texture* texture = nullptr;
    std::string dialogueText = "Привет! Нажми E ещё раз, чтобы закрыть.";

    NPC1(SDL_Renderer* renderer, const std::string& texPath, float x, float y);
    ~NPC1();

    void draw(float camX, float camY, float zoom);
    bool isPlayerNear(float playerX, float playerY, float dist = 60.0f);

private:
    SDL_Renderer* renderer = nullptr;   // <-- добавить
};