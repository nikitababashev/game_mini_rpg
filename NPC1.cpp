#include "NPC1.h"
#include <SDL3/SDL_log.h>

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

    initAnimations();

    src.x = 0.0f;
    src.y = 0.0f;
    src.w = 46.0f;
    src.h = 55.0f;
    dest.w = width;
    dest.h = height;

    // Первый диалог - амулет
    dialogueLines = {
        "Привет! Ты попал на чудо остров!",
        "Хочешь, подарю тебе амулет?"
    };
    option1 = "Да(1)";
    option2 = "Нет(2)";

    // Второй диалог - квест на поиск цветка
    questStartLines = {
        "О, я вижу у тебя есть амулет!",
        "В глубине леса растёт легендарный Золотой Цветок.",
        "Принеси мне его, и я дам тебе магический кристалл!"
    };
    questProgressLines = {
        "Ты ещё не нашёл Золотой Цветок?",
        "Поищи в самой глубокой части леса..."
    };
    questCompleteLines = {
        "Ты нашёл Золотой Цветок! Великолепно!",
        "Держи обещанную награду - магический кристалл!"
    };
    questOption1 = "Взять задание(1)";
    questOption2 = "Отказаться(2)";
}

void NPC1::showAnimationnpc(animationnpc animation, int now, int delay) {
    if (animation.frames <= 0 || animation.animationDelay <= 0) {
        return;
    }

    if (delay >= animation.animationDelay) {
        lastUpdate = now;
        currentIndex = (currentIndex + 1) % animation.frames;
        src.x = (float)(currentIndex * 46);
        src.y = (float)(55 * animation.y);
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
    Uint64 now = SDL_GetTicks();
    int delay = (int)(now - lastUpdate);
    showAnimationnpc(animations.idle, (int)now, delay);
}

void NPC1::initAnimations() {
    animations.idle.frames = 10;
    animations.idle.animationDelay = 100;
    animations.idle.y = 0;
}

bool NPC1::isPlayerNear(float playerX, float playerY, float dist) {
    float dx = worldX - playerX;
    float dy = worldY - playerY;
    return (dx * dx + dy * dy) <= (dist * dist);
}

void NPC1::setHasItemCallback(std::function<bool(const std::string&)> callback) {
    hasItemCallback = callback;
}

void NPC1::setRemoveItemCallback(std::function<bool(const std::string&, int)> callback) {
    removeItemCallback = callback;
}

void NPC1::setAddItemCallback(std::function<bool(const std::string&, int)> callback) {
    addItemCallback = callback;
}

bool NPC1::hasAmulet() const {
    if (hasItemCallback) {
        return hasItemCallback("Magic Amulet");
    }
    return gaveItem;
}

bool NPC1::hasQuestItem() const {
    if (hasItemCallback) {
        return hasItemCallback("Golden Flower");
    }
    return false;
}