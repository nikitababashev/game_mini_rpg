#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <vector>
#include <functional>

class DialogManager {
public:
    bool isWaitingChoice() const { return waitingChoice; }
    // Показать диалог: последовательные фразы, затем два варианта выбора
    void show(const std::vector<std::string>& lines,
        const std::string& choice1, std::function<void()> action1,
        const std::string& choice2, std::function<void()> action2,
        TTF_Font* font, SDL_Renderer* renderer);

    void hide();
    bool isActive() const { return active; }

    // Перейти к следующей фразе (вызов по E)
    void next();

    // Обработать выбор (1 или 2)
    void handleChoice(int choice);

    void draw(SDL_Renderer* renderer);

private:
    bool active = false;
    bool waitingChoice = false;          // true, когда показываем варианты
    std::vector<std::string> lines;
    size_t currentLine = 0;

    std::string choice1Text, choice2Text;
    std::function<void()> action1, action2;

    SDL_Texture* texture = nullptr;      // текущая отображаемая текстура
    TTF_Font* font = nullptr;
    SDL_Renderer* renderer = nullptr;

    void createTexture(const std::string& text);
};