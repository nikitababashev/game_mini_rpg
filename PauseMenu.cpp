#include "PauseMenu.h"
#include <SDL3/SDL_log.h>
#include <cmath>

PauseMenu::PauseMenu() {}

PauseMenu::~PauseMenu() {
    cleanup();
}

bool PauseMenu::init(SDL_Renderer* ren, TTF_Font* fnt) {
    renderer = ren;
    font = fnt;

    if (!renderer || !font) {
        SDL_Log("PauseMenu::init: renderer or font is null");
        return false;
    }

    // Создаём меню
    menuItems = {
        {"Resume Game", MenuOption::RESUME},
        {"Music Settings", MenuOption::MUSIC_SETTINGS},
        {"Exit to Menu", MenuOption::EXIT_TO_MENU},
        {"Exit to Desktop", MenuOption::EXIT_TO_DESKTOP}
    };

    // Создаём текстуры для пунктов меню
    for (size_t i = 0; i < menuItems.size(); ++i) {
        createMenuItemTexture((int)i);
    }

    // Создаём фон панели
    panelTexture = createColoredTexture((int)MENU_WIDTH, (int)MENU_HEIGHT, { 30, 30, 40, 220 });

    // Создаём текстуру выделенного пункта
    selectedBgTexture = createColoredTexture((int)(MENU_WIDTH - 60), (int)OPTION_HEIGHT, { 255, 215, 0, 100 });

    // Создаём текстуры для ползунка
    sliderBgTexture = createColoredTexture(300, 8, { 100, 100, 100, 200 });
    sliderHandleTexture = createColoredTexture(20, 30, { 255, 215, 0, 255 });

    // Иконка громкости (простой текст)
    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Surface* surf = TTF_RenderText_Blended(font, "Volume", 0, white);
    if (surf) {
        volumeIconTexture = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_DestroySurface(surf);
    }

    selectedIndex = 0;
    active = false;
    inMusicSettings = false;

    return true;
}

void PauseMenu::cleanup() {
    for (auto& item : menuItems) {
        if (item.textTexture) {
            SDL_DestroyTexture(item.textTexture);
            item.textTexture = nullptr;
        }
    }

    if (panelTexture) SDL_DestroyTexture(panelTexture);
    if (selectedBgTexture) SDL_DestroyTexture(selectedBgTexture);
    if (sliderBgTexture) SDL_DestroyTexture(sliderBgTexture);
    if (sliderHandleTexture) SDL_DestroyTexture(sliderHandleTexture);
    if (volumeIconTexture) SDL_DestroyTexture(volumeIconTexture);

    panelTexture = nullptr;
    selectedBgTexture = nullptr;
    sliderBgTexture = nullptr;
    sliderHandleTexture = nullptr;
    volumeIconTexture = nullptr;
}

SDL_Texture* PauseMenu::createColoredTexture(int width, int height, SDL_Color color) {
    SDL_Surface* surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_ARGB8888);
    if (!surface) {
        SDL_Log("Failed to create surface: %s", SDL_GetError());
        return nullptr;
    }

    SDL_FillSurfaceRect(surface, nullptr, SDL_MapSurfaceRGBA(surface, color.r, color.g, color.b, color.a));

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    return texture;
}

void PauseMenu::createMenuItemTexture(int index) {
    if (index < 0 || index >= (int)menuItems.size()) return;

    if (menuItems[index].textTexture) {
        SDL_DestroyTexture(menuItems[index].textTexture);
    }

    SDL_Color color = { 255, 255, 255, 255 };
    SDL_Surface* surf = TTF_RenderText_Blended(font, menuItems[index].text.c_str(), 0, color);
    if (surf) {
        menuItems[index].textTexture = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_GetTextureSize(menuItems[index].textTexture, &menuItems[index].textW, &menuItems[index].textH);
        SDL_DestroySurface(surf);
    }
}

void PauseMenu::open() {
    active = true;
    inMusicSettings = false;
    selectedIndex = 0;
    targetVolume = currentVolume;
}

void PauseMenu::close() {
    active = false;
    inMusicSettings = false;
    volumeDragging = false;
}

void PauseMenu::setVolumeCallback(std::function<void(float)> callback) {
    onVolumeChange = callback;
}

void PauseMenu::setExitToMenuCallback(std::function<void()> callback) {
    onExitToMenu = callback;
}

void PauseMenu::setExitToDesktopCallback(std::function<void()> callback) {
    onExitToDesktop = callback;
}

void PauseMenu::update(float deltaTime) {
    // Плавное изменение громкости
    if (std::abs(currentVolume - targetVolume) > 0.01f) {
        currentVolume += (targetVolume - currentVolume) * deltaTime * 10.0f;
        if (onVolumeChange) {
            onVolumeChange(currentVolume);
        }
    }
}

void PauseMenu::handleEvent(const SDL_Event& event) {
    if (!active) return;

    if (inMusicSettings) {
        handleMusicSettingsInput(event);
    }
    else {
        handleMainMenuInput(event);
    }
}

void PauseMenu::handleMainMenuInput(const SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.key) {
        case SDLK_UP:
            selectedIndex = (selectedIndex - 1 + OPTION_COUNT) % OPTION_COUNT;
            break;
        case SDLK_DOWN:
            selectedIndex = (selectedIndex + 1) % OPTION_COUNT;
            break;
        case SDLK_RETURN:
        case SDLK_SPACE:
            switch (menuItems[selectedIndex].action) {
            case MenuOption::RESUME:
                close();
                break;
            case MenuOption::MUSIC_SETTINGS:
                inMusicSettings = true;
                break;
            case MenuOption::EXIT_TO_MENU:
                if (onExitToMenu) onExitToMenu();
                close();
                break;
            case MenuOption::EXIT_TO_DESKTOP:
                if (onExitToDesktop) onExitToDesktop();
                break;
            }
            break;
        case SDLK_ESCAPE:
            close();
            break;
        }
    }
}

void PauseMenu::handleMusicSettingsInput(const SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.key) {
        case SDLK_LEFT:
            targetVolume = std::max(0.0f, targetVolume - 0.1f);
            break;
        case SDLK_RIGHT:
            targetVolume = std::min(1.0f, targetVolume + 0.1f);
            break;
        case SDLK_ESCAPE:
            inMusicSettings = false;
            break;
        case SDLK_RETURN:
            inMusicSettings = false;
            break;
        }
    }
    else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
        // Проверка клика по ползунку
        float mouseX = (float)event.button.x;
        float mouseY = (float)event.button.y;

        int screenW, screenH;
        SDL_GetWindowSize(window, &screenW, &screenH);

        float sliderX = (float)screenW / 2.0f - 150.0f;
        float sliderY = (float)screenH / 2.0f + 50.0f;
        float sliderW = 300.0f;

        if (mouseX >= sliderX && mouseX <= sliderX + sliderW &&
            mouseY >= sliderY - 15 && mouseY <= sliderY + 25) {
            volumeDragging = true;
            float t = (mouseX - sliderX) / sliderW;
            targetVolume = std::max(0.0f, std::min(1.0f, t));
        }
    }
    else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
        volumeDragging = false;
    }
    else if (event.type == SDL_EVENT_MOUSE_MOTION && volumeDragging) {
        float mouseX = (float)event.motion.x;
        int screenW, screenH;
        SDL_GetWindowSize(window, &screenW, &screenH);

        float sliderX = (float)screenW / 2.0f - 150.0f;
        float sliderW = 300.0f;

        float t = (mouseX - sliderX) / sliderW;
        targetVolume = std::max(0.0f, std::min(1.0f, t));
    }
}

void PauseMenu::draw(float screenWidth, float screenHeight) {
    if (!active) return;

    // Затемнение фона
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect bgRect = { 0, 0, screenWidth, screenHeight };
    SDL_RenderFillRect(renderer, &bgRect);

    if (inMusicSettings) {
        renderMusicSettings();
    }
    else {
        renderMainMenu();
    }
}

void PauseMenu::renderMainMenu() {
    float screenW = 1920.0f;
    float screenH = 1080.0f;
    float centerX = screenW / 2.0f;
    float centerY = screenH / 2.0f;

    // Рисуем панель
    SDL_FRect panelRect = { centerX - MENU_WIDTH / 2.0f, centerY - MENU_HEIGHT / 2.0f, MENU_WIDTH, MENU_HEIGHT };
    if (panelTexture) {
        SDL_RenderTexture(renderer, panelTexture, nullptr, &panelRect);
    }

    // Рисуем заголовок
    std::string title = "PAUSED";
    SDL_Color titleColor = { 255, 215, 0, 255 };
    SDL_Surface* titleSurf = TTF_RenderText_Blended(font, title.c_str(), 0, titleColor);
    if (titleSurf) {
        SDL_Texture* titleTex = SDL_CreateTextureFromSurface(renderer, titleSurf);
        float titleW, titleH;
        SDL_GetTextureSize(titleTex, &titleW, &titleH);
        SDL_FRect titleRect = { centerX - titleW / 2.0f, centerY - MENU_HEIGHT / 2.0f + 30.0f, titleW, titleH };
        SDL_RenderTexture(renderer, titleTex, nullptr, &titleRect);
        SDL_DestroyTexture(titleTex);
        SDL_DestroySurface(titleSurf);
    }

    // Рисуем пункты меню
    float startY = centerY - MENU_HEIGHT / 2.0f + 120.0f;
    for (int i = 0; i < OPTION_COUNT; ++i) {
        float optionX = centerX - (MENU_WIDTH - 60) / 2.0f;
        float optionY = startY + i * (OPTION_HEIGHT + 10.0f);

        // Подсветка выбранного пункта
        if (i == selectedIndex && selectedBgTexture) {
            SDL_FRect selectedRect = { optionX, optionY, MENU_WIDTH - 60, OPTION_HEIGHT };
            SDL_RenderTexture(renderer, selectedBgTexture, nullptr, &selectedRect);
        }

        // Текст
        if (menuItems[i].textTexture) {
            float textX = centerX - menuItems[i].textW / 2.0f;
            float textY = optionY + (OPTION_HEIGHT - menuItems[i].textH) / 2.0f;
            SDL_FRect textRect = { textX, textY, menuItems[i].textW, menuItems[i].textH };
            SDL_RenderTexture(renderer, menuItems[i].textTexture, nullptr, &textRect);
        }
    }
}

void PauseMenu::renderMusicSettings() {
    float screenW = 1920.0f;
    float screenH = 1080.0f;
    float centerX = screenW / 2.0f;
    float centerY = screenH / 2.0f;

    // Рисуем панель
    float panelW = 500.0f;
    float panelH = 300.0f;
    SDL_FRect panelRect = { centerX - panelW / 2.0f, centerY - panelH / 2.0f, panelW, panelH };
    if (panelTexture) {
        SDL_RenderTexture(renderer, panelTexture, nullptr, &panelRect);
    }

    // Заголовок
    std::string title = "MUSIC SETTINGS";
    SDL_Color titleColor = { 255, 215, 0, 255 };
    SDL_Surface* titleSurf = TTF_RenderText_Blended(font, title.c_str(), 0, titleColor);
    if (titleSurf) {
        SDL_Texture* titleTex = SDL_CreateTextureFromSurface(renderer, titleSurf);
        float titleW, titleH;
        SDL_GetTextureSize(titleTex, &titleW, &titleH);
        SDL_FRect titleRect = { centerX - titleW / 2.0f, centerY - panelH / 2.0f + 30.0f, titleW, titleH };
        SDL_RenderTexture(renderer, titleTex, nullptr, &titleRect);
        SDL_DestroyTexture(titleTex);
        SDL_DestroySurface(titleSurf);
    }

    // Текст "Volume:"
    std::string volumeText = "Master Volume";
    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Surface* volSurf = TTF_RenderText_Blended(font, volumeText.c_str(), 0, white);
    if (volSurf) {
        SDL_Texture* volTex = SDL_CreateTextureFromSurface(renderer, volSurf);
        float volW, volH;
        SDL_GetTextureSize(volTex, &volW, &volH);
        SDL_FRect volRect = { centerX - 200.0f, centerY - 30.0f, volW, volH };
        SDL_RenderTexture(renderer, volTex, nullptr, &volRect);
        SDL_DestroyTexture(volTex);
        SDL_DestroySurface(volSurf);
    }

    // Ползунок громкости
    float sliderX = centerX - 150.0f;
    float sliderY = centerY + 20.0f;
    float sliderW = 300.0f;
    float sliderH = 8.0f;

    if (sliderBgTexture) {
        SDL_FRect sliderRect = { sliderX, sliderY, sliderW, sliderH };
        SDL_RenderTexture(renderer, sliderBgTexture, nullptr, &sliderRect);
    }

    // Заполненная часть ползунка
    float filledWidth = sliderW * targetVolume;
    if (filledWidth > 0) {
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
        SDL_FRect filledRect = { sliderX, sliderY, filledWidth, sliderH };
        SDL_RenderFillRect(renderer, &filledRect);
    }

    // Ручка ползунка
    if (sliderHandleTexture) {
        float handleX = sliderX + filledWidth - 10.0f;
        float handleY = sliderY - 11.0f;
        float handleW, handleH;
        SDL_GetTextureSize(sliderHandleTexture, &handleW, &handleH);
        SDL_FRect handleRect = { handleX, handleY, handleW, handleH };
        SDL_RenderTexture(renderer, sliderHandleTexture, nullptr, &handleRect);
    }

    // Значение громкости в процентах
    std::string percentText = std::to_string((int)(targetVolume * 100)) + "%";
    SDL_Surface* percentSurf = TTF_RenderText_Blended(font, percentText.c_str(), 0, white);
    if (percentSurf) {
        SDL_Texture* percentTex = SDL_CreateTextureFromSurface(renderer, percentSurf);
        float percentW, percentH;
        SDL_GetTextureSize(percentTex, &percentW, &percentH);
        SDL_FRect percentRect = { sliderX + sliderW + 20.0f, sliderY - percentH / 2.0f + 4.0f, percentW, percentH };
        SDL_RenderTexture(renderer, percentTex, nullptr, &percentRect);
        SDL_DestroyTexture(percentTex);
        SDL_DestroySurface(percentSurf);
    }

    // Подсказка
    std::string hint = "Press ESC to go back";
    SDL_Surface* hintSurf = TTF_RenderText_Blended(font, hint.c_str(), 0, white);
    if (hintSurf) {
        SDL_Texture* hintTex = SDL_CreateTextureFromSurface(renderer, hintSurf);
        float hintW, hintH;
        SDL_GetTextureSize(hintTex, &hintW, &hintH);
        SDL_FRect hintRect = { centerX - hintW / 2.0f, centerY + panelH / 2.0f - 50.0f, hintW, hintH };
        SDL_RenderTexture(renderer, hintTex, nullptr, &hintRect);
        SDL_DestroyTexture(hintTex);
        SDL_DestroySurface(hintSurf);
    }
}