#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <vector>
#include <string>

struct animationnpc {
    int frames = 0;
    int animationDelay = 0;
    int y = 0;
};

struct animationDatanpc {
    animationnpc idle;
    // ... другие анимации
};

class NPC1 {
public:
    NPC1(SDL_Renderer* renderer, const std::string& texPath, float x, float y);
    ~NPC1();

    void update();
    void draw(float camX, float camY, float zoom);
    void draw();                          // если нужен
    void handleEvents();
    bool isPlayerNear(float playerX, float playerY, float dist = 60.0f);

    float worldX, worldY;
    float width = 46, height = 55;        // реальные размеры спрайта

    // НОВЫЕ ПОЛЯ ДЛЯ ДИАЛОГОВ
    std::vector<std::string> dialogueLines;
    std::string option1;
    std::string option2;
    bool gaveItem = false;

private:
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    SDL_FRect src;
    SDL_FRect dest;
    int currentIndex = 0;
    Uint64 lastUpdate = 0;
    animationDatanpc animations;

    void initAnimations();
    void showAnimationnpc(animationnpc animation, int now, int delay);
};