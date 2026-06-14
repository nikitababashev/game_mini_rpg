#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <functional>

enum class CharacterGender {
    MALE,
    FEMALE
};

class CharacterSelect {
public:
    CharacterSelect();
    ~CharacterSelect();

    bool init(SDL_Renderer* renderer, SDL_Window* window, TTF_Font* font,
        SDL_Texture* maleTex, SDL_Texture* femaleTex);
    void cleanup();

    void open();
    void close();
    bool isOpen() const { return active; }

    void handleEvent(const SDL_Event& event);
    void draw();

    CharacterGender getSelectedGender() const { return selectedGender; }
    void setOnConfirmCallback(std::function<void(CharacterGender)> callback);

private:
    SDL_Renderer* renderer = nullptr;
    SDL_Window* window = nullptr;
    TTF_Font* font = nullptr;

    SDL_Texture* maleTexture = nullptr;
    SDL_Texture* femaleTexture = nullptr;

    CharacterGender selectedGender = CharacterGender::MALE;
    bool active = false;

    std::function<void(CharacterGender)> onConfirm;

    // Прямоугольники для кнопок
    SDL_FRect maleRect;
    SDL_FRect femaleRect;
    SDL_FRect confirmRect;
};