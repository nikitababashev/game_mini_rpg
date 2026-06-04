#include "DialogManager.h"
#include <SDL3/SDL_log.h>

void DialogManager::show(const std::string& text, TTF_Font* font, SDL_Renderer* renderer) {
    // Если уже показывается другой диалог – скрываем старый
    hide();
    active = true;
    currentText = text;
    currentFont = font;

    // Создаём текстуру с текстом
    SDL_Color color = { 255, 255, 255, 255 };
    SDL_Surface* surf = TTF_RenderText_Blended(font, text.c_str(), 0, color);
    if (surf) {
        texture = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_DestroySurface(surf);
    }
}

void DialogManager::hide() {
    active = false;
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}

void DialogManager::draw(SDL_Renderer* renderer) {
    if (!active || !texture) return;

    // Полупрозрачная панель
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_FRect panel = { 50.0f, 700.0f, 1820.0f, 280.0f };
    SDL_RenderFillRect(renderer, &panel);

    // Текст по центру панели
    float texW, texH;
    SDL_GetTextureSize(texture, &texW, &texH);
    SDL_FRect textDest = {
        panel.x + 20.0f,
        panel.y + (panel.h - texH) / 2.0f,
        texW, texH
    };
    SDL_RenderTexture(renderer, texture, NULL, &textDest);
}