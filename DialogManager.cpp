#include "DialogManager.h"
#include <SDL3/SDL_log.h>

void DialogManager::show(const std::vector<std::string>& lines,
    const std::string& choice1, std::function<void()> act1,
    const std::string& choice2, std::function<void()> act2,
    TTF_Font* font, SDL_Renderer* renderer) {
    hide();
    if (lines.empty()) return;

    this->lines = lines;
    currentLine = 0;
    choice1Text = choice1;
    choice2Text = choice2;
    action1 = act1;
    action2 = act2;
    waitingChoice = false;
    this->font = font;
    this->renderer = renderer;
    active = true;

    // Показываем первую фразу
    createTexture(lines[0]);
}

void DialogManager::hide() {
    active = false;
    waitingChoice = false;
    lines.clear();
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}

void DialogManager::next() {
    if (!active || waitingChoice) return;

    currentLine++;
    if (currentLine < lines.size()) {
        // Следующая фраза
        createTexture(lines[currentLine]);
    }
    else {
        // Если есть варианты выбора - показываем их
        if (!choice1Text.empty() || !choice2Text.empty()) {
            waitingChoice = true;
            std::string combined = choice1Text + "   или   " + choice2Text;
            createTexture(combined);
        }
        else {
            // Нет вариантов выбора - просто закрываем диалог
            hide();
        }
    }
}

void DialogManager::handleChoice(int choice) {
    if (!active || !waitingChoice) return;

    if (choice == 1 && action1) action1();
    else if (choice == 2 && action2) action2();

    hide();
}

void DialogManager::draw(SDL_Renderer* renderer) {
    if (!active || !texture) return;

    // Панель
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_FRect panel = { 50.0f, 700.0f, 1820.0f, 280.0f };
    SDL_RenderFillRect(renderer, &panel);

    float texW, texH;
    SDL_GetTextureSize(texture, &texW, &texH);
    SDL_FRect textDest = {
        panel.x + 20.0f,
        panel.y + (panel.h - texH) / 2.0f,
        texW, texH
    };
    SDL_RenderTexture(renderer, texture, NULL, &textDest);
}

void DialogManager::createTexture(const std::string& text) {
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
    SDL_Color color = { 255, 255, 255, 255 };
    SDL_Surface* surf = TTF_RenderText_Blended(font, text.c_str(), 0, color);
    if (surf) {
        texture = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_DestroySurface(surf);
    }
    else {
        SDL_Log("DialogManager: ошибка создания текстуры");
    }
}