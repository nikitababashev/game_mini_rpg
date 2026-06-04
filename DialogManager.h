#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>

class DialogManager {
public:
    // Показать диалог с указанным текстом
    void show(const std::string& text, TTF_Font* font, SDL_Renderer* renderer);
    // Скрыть диалог
    void hide();
    // Активен ли сейчас диалог
    bool isActive() const { return active; }
    // Нарисовать диалоговое окно (если активно)
    void draw(SDL_Renderer* renderer);

private:
    bool active = false;
    std::string currentText;
    SDL_Texture* texture = nullptr;
    TTF_Font* currentFont = nullptr;   // запоминаем, каким шрифтом создали текстуру
};