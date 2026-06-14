#include "CharacterSelect.h"
#include <SDL3/SDL_log.h>

CharacterSelect::CharacterSelect() {}

CharacterSelect::~CharacterSelect() {
    cleanup();
}

bool CharacterSelect::init(SDL_Renderer* ren, SDL_Window* win, TTF_Font* fnt,
    SDL_Texture* maleTex, SDL_Texture* femaleTex) {
    renderer = ren;
    window = win;
    font = fnt;
    maleTexture = maleTex;
    femaleTexture = femaleTex;

    if (!renderer || !window || !font) {
        SDL_Log("CharacterSelect init failed");
        return false;
    }

    // Задаём прямоугольники для кнопок (на весь экран 1920x1080)
    maleRect = { 460, 340, 200, 200 };
    femaleRect = { 1260, 340, 200, 200 };
    confirmRect = { 860, 650, 200, 60 };

    SDL_Log("CharacterSelect initialized");
    return true;
}

void CharacterSelect::cleanup() {
    // Не удаляем текстуры - они принадлежат main.cpp
}

void CharacterSelect::open() {
    active = true;
    selectedGender = CharacterGender::MALE;
    SDL_Log("CharacterSelect opened");
}

void CharacterSelect::close() {
    active = false;
    SDL_Log("CharacterSelect closed");
}

void CharacterSelect::setOnConfirmCallback(std::function<void(CharacterGender)> callback) {
    onConfirm = callback;
}

void CharacterSelect::handleEvent(const SDL_Event& event) {
    if (!active) return;

    SDL_Log("CharacterSelect handling event type: %d", event.type);

    // Обработка клавиатуры
    if (event.type == SDL_EVENT_KEY_DOWN) {
        SDL_Log("Key pressed: %d", event.key.key);

        switch (event.key.key) {
        case SDLK_LEFT:
            selectedGender = CharacterGender::MALE;
            SDL_Log("Selected MALE");
            break;
        case SDLK_RIGHT:
            selectedGender = CharacterGender::FEMALE;
            SDL_Log("Selected FEMALE");
            break;
        case SDLK_RETURN:
            SDL_Log("ENTER pressed");
            if (onConfirm) {
                onConfirm(selectedGender);
            }
            close();
            break;
        case SDLK_ESCAPE:
            close();
            break;
        }
    }

    // Обработка мыши
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        float x = (float)event.button.x;
        float y = (float)event.button.y;
        SDL_Log("Mouse click at: %.0f, %.0f", x, y);

        // Проверяем клик по мужскому персонажу
        if (x >= maleRect.x && x <= maleRect.x + maleRect.w &&
            y >= maleRect.y && y <= maleRect.y + maleRect.h) {
            selectedGender = CharacterGender::MALE;
            SDL_Log("Clicked MALE");
        }
        // Проверяем клик по женскому персонажу
        else if (x >= femaleRect.x && x <= femaleRect.x + femaleRect.w &&
            y >= femaleRect.y && y <= femaleRect.y + femaleRect.h) {
            selectedGender = CharacterGender::FEMALE;
            SDL_Log("Clicked FEMALE");
        }
        // Проверяем клик по кнопке подтверждения
        else if (x >= confirmRect.x && x <= confirmRect.x + confirmRect.w &&
            y >= confirmRect.y && y <= confirmRect.y + confirmRect.h) {
            SDL_Log("Clicked CONFIRM");
            if (onConfirm) {
                onConfirm(selectedGender);
            }
            close();
        }
    }
}

void CharacterSelect::draw() {
    if (!active) return;

    // Получаем размеры окна
    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    // Затемнение фона
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_FRect bgRect = { 0, 0, (float)w, (float)h };
    SDL_RenderFillRect(renderer, &bgRect);

    // Рисуем рамку для выбранного персонажа
    SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
    if (selectedGender == CharacterGender::MALE) {
        SDL_FRect border = { maleRect.x - 5, maleRect.y - 5, maleRect.w + 10, maleRect.h + 10 };
        SDL_RenderRect(renderer, &border);
    }
    else {
        SDL_FRect border = { femaleRect.x - 5, femaleRect.y - 5, femaleRect.w + 10, femaleRect.h + 10 };
        SDL_RenderRect(renderer, &border);
    }

    // Рисуем персонажей
    if (maleTexture) {
        SDL_RenderTexture(renderer, maleTexture, NULL, &maleRect);
    }
    else {
        SDL_Log("maleTexture is null!");
    }

    if (femaleTexture) {
        SDL_RenderTexture(renderer, femaleTexture, NULL, &femaleRect);
    }
    else {
        SDL_Log("femaleTexture is null!");
    }

    // Рисуем кнопку подтверждения
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &confirmRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &confirmRect);

    // Рисуем текст на кнопке (просто через SDL_Log, для простоты)
    // Вместо TTF_RenderText_Blended используем простой прямоугольник с текстом через дебаг

    SDL_Log("CharacterSelect drawing complete");
}