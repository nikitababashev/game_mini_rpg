#include "NPC1.h"
#include <SDL3/SDL_log.h>   // для SDL_Log

void NPC1::draw() {
    SDL_RenderTexture(renderer, texture, &src, &dest);
}

void NPC1::handleEvents() {
}

NPC1::NPC1(SDL_Renderer* renderer, const std::string& texPath, float x, float y)
    : renderer(renderer), worldX(x), worldY(y), currentIndex(0), lastUpdate(0)
{
    texture = IMG_LoadTexture(renderer, texPath.c_str());
    if (!texture) SDL_Log("NPC1 texture error: %s", SDL_GetError());

    // Инициализация анимации
    initAnimations();

    // Инициализация src для спрайтов
    src.x = 0;
    src.y = 0;
    src.w = 46;  // ширина кадра
    src.h = 55;  // высота кадра
    dest.w = width;
    dest.h = height;
}

void NPC1::showAnimationnpc(animationnpc animation, int now, int delay) {
    // ЗАЩИТА ОТ ДЕЛЕНИЯ НА НОЛЬ
    if (animation.frames <= 0 || animation.animationDelay <= 0) {
        return;  // Нельзя делить на ноль
    }

    if (delay >= animation.animationDelay) {
        lastUpdate = now;
        currentIndex = (currentIndex + 1) % animation.frames;
        src.x = currentIndex * 46;
        src.y = 55 * animation.y;
    }
}

NPC1::~NPC1() {
    if (texture) SDL_DestroyTexture(texture);
}

void NPC1::draw(float camX, float camY, float zoom) {
    float drawX = (worldX - width / 2.0f - camX) * zoom;
    float drawY = (worldY - height / 2.0f - camY) * zoom;
    SDL_FRect dest = { drawX, drawY, width * 2, height * 2 };
    SDL_FRect srcRect = { src.x, src.y, src.w, src.h };
    SDL_RenderTexture(renderer, texture, &srcRect, &dest);
}

void NPC1::update() {
    Uint64 now = SDL_GetTicks();  // время инициализации и текущее
    int delay = (int)(now - lastUpdate);

    // Используем animations.idle, а не просто idle
    showAnimationnpc(animations.idle, (int)now, delay);
}

void NPC1::initAnimations() {
    // Инициализация анимации IDLE
    animations.idle.frames = 10;        // количество кадров
    animations.idle.animationDelay = 100; // задержка между кадрами (мс)
    animations.idle.y = 0;              // строка в спрайтшите
}

bool NPC1::isPlayerNear(float playerX, float playerY, float dist) {   // убрано = 60.0f
    float dx = worldX - playerX;
    float dy = worldY - playerY;
    return (dx * dx + dy * dy) <= (dist * dist);
}