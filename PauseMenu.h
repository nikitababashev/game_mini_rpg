#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <functional>
#include <vector>
#include <string>

enum class MenuOption {
    RESUME,
    MUSIC_SETTINGS,
    EXIT_TO_MENU,
    EXIT_TO_DESKTOP
};

class PauseMenu {
public:
    PauseMenu();
    ~PauseMenu();

    bool init(SDL_Renderer* renderer, TTF_Font* font);
    void cleanup();

    void open();
    void close();
    bool isOpen() const { return active; }

    void handleEvent(const SDL_Event& event);
    void draw(float screenWidth, float screenHeight);

    // Callback для управления музыкой
    void setVolumeCallback(std::function<void(float)> callback);
    void setExitToMenuCallback(std::function<void()> callback);
    void setExitToDesktopCallback(std::function<void()> callback);

    void update(float deltaTime);
    void setWindow(SDL_Window* win) { window = win; }

private:
    static constexpr int OPTION_COUNT = 4;
    static constexpr float MENU_WIDTH = 500.0f;
    static constexpr float MENU_HEIGHT = 500.0f;
    static constexpr float OPTION_HEIGHT = 60.0f;
    static constexpr float OPTION_PADDING = 20.0f;
    SDL_Window* window = nullptr;

    struct MenuItem {
        std::string text;
        MenuOption action;
        SDL_Texture* textTexture = nullptr;
        float textW = 0, textH = 0;
    };

    std::vector<MenuItem> menuItems;
    int selectedIndex = 0;
    bool active = false;
    bool inMusicSettings = false;

    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;

    // Фон меню
    SDL_Texture* panelTexture = nullptr;
    SDL_Texture* selectedBgTexture = nullptr;

    // Настройки музыки
    float currentVolume = 1.0f;
    float targetVolume = 1.0f;
    bool volumeDragging = false;

    // Текстуры для ползунка громкости
    SDL_Texture* sliderBgTexture = nullptr;
    SDL_Texture* sliderHandleTexture = nullptr;
    SDL_Texture* volumeIconTexture = nullptr;

    // Callbacks
    std::function<void(float)> onVolumeChange;
    std::function<void()> onExitToMenu;
    std::function<void()> onExitToDesktop;

    // Вспомогательные методы
    void createMenuItemTexture(int index);
    void updateSelectedTexture();
    void renderMainMenu();
    void renderMusicSettings();
    void handleMainMenuInput(const SDL_Event& event);
    void handleMusicSettingsInput(const SDL_Event& event);
    SDL_Texture* createColoredTexture(int width, int height, SDL_Color color);
};