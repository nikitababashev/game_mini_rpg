#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>

struct animationnpc {
    int frames;
    int animationDelay;
    int y;
};

struct animationDatanpc {
    animationnpc idle;
};

class NPC1 {
public:
    void draw();
    void update();
    void handleEvents();
    float worldX, worldY;
    float width = 64, height = 64;
    SDL_Texture* texture = nullptr;
    std::string dialogueText = "Привет! Нажми E ещё раз, чтобы закрыть.";

    NPC1(SDL_Renderer* renderer, const std::string& texPath, float x, float y);
    ~NPC1();

    void draw(float camX, float camY, float zoom);
    bool isPlayerNear(float playerX, float playerY, float dist = 60.0f);

private:
    void initAnimations();
    void showAnimationnpc(animationnpc animation, int now, int delay);
    SDL_Renderer* renderer = nullptr;
    SDL_FRect dest;
    SDL_FRect src;
    int speed = 0;
    int walk_speed = 0;
    animationnpc idle;          // можно удалить, если не используется
    animationDatanpc animations;
    int currentIndex = 0;
    int lastUpdate = 0;
    float sizeSprite = 0;
    int isIdle = 0;
    int isWalk = 0;
    int where_see_idle = 0;
    SDL_FlipMode flip = SDL_FLIP_NONE;
};