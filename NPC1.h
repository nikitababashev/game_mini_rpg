#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <vector>
#include <string>
#include <functional>

struct animationnpc {
    int frames = 0;
    int animationDelay = 0;
    int y = 0;
};

struct animationDatanpc {
    animationnpc idle;
};

class NPC1 {
public:
    NPC1(SDL_Renderer* renderer, const std::string& texPath, float x, float y);
    ~NPC1();

    void update();
    void draw(float camX, float camY, float zoom);
    void draw();
    void handleEvents();
    bool isPlayerNear(float playerX, float playerY, float dist = 60.0f);

    float worldX, worldY;
    float width = 46, height = 55;

    // Первый диалог (получение амулета)
    std::vector<std::string> dialogueLines;
    std::string option1;
    std::string option2;
    bool gaveItem = false;

    // Второй диалог (квест после получения амулета)
    std::vector<std::string> questStartLines;
    std::vector<std::string> questProgressLines;
    std::vector<std::string> questCompleteLines;
    std::string questOption1;
    std::string questOption2;

    // Состояние квеста
    bool questActive = false;
    bool questCompleted = false;
    bool questRewarded = false;

    // Проверка инвентаря
    void setHasItemCallback(std::function<bool(const std::string&)> callback);
    void setRemoveItemCallback(std::function<bool(const std::string&, int)> callback);
    void setAddItemCallback(std::function<bool(const std::string&, int)> callback);

    // Проверка наличия амулета у игрока
    bool hasAmulet() const;
    bool hasQuestItem() const;

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

    std::function<bool(const std::string&)> hasItemCallback;
    std::function<bool(const std::string&, int)> removeItemCallback;
    std::function<bool(const std::string&, int)> addItemCallback;
};