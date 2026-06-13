#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "Player.h"
#include "Resource.h"
#include "json.hpp"
#include "GameState.h"
#include "TileMap.h"
#include "NPC1.h"
#include "Camera.h"
#include "DialogManager.h"
#include "AudioManager.h"
#include "Inventory.h"
#include "PauseMenu.h"

using json = nlohmann::json;

// Глобальные объекты
static Inventory playerInventory;
static PauseMenu pauseMenu;
static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;

Camera camera;
DialogManager dialogMgr;
AudioManager audioManager;

Player* player = nullptr;
Resource* thing = nullptr;
TileMap* tileMap = nullptr;

// Текстуры и шрифты UI
SDL_Texture* menuBackground = nullptr;
TTF_Font* menuFont = nullptr;
TTF_Font* dialogFont = nullptr;
SDL_Texture* menuTextTexture = nullptr;
SDL_Texture* disclaimerBG = nullptr;
SDL_Texture* extraImage = nullptr;
SDL_Texture* skipTextTexture = nullptr;

NPC1* dialogueNPC = nullptr;

// Флаг для предотвращения повторного добавления амулета
bool amuletReceived = false;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    SDL_CreateWindowAndRenderer("beautiful_garden", 1920, 1080, 0, &window, &renderer);
    pauseMenu.setWindow(window);
    TTF_Init();

    // Инициализация аудио
    audioManager.init();

    // Загрузка шрифтов
    menuFont = TTF_OpenFont("assets/Banty Bold.ttf", 72);
    if (!menuFont) SDL_Log("Ошибка загрузки menuFont: %s", SDL_GetError());

    dialogFont = TTF_OpenFont("assets/DejaVuSans.ttf", 48);
    if (!dialogFont) SDL_Log("Ошибка загрузки dialogFont: %s", SDL_GetError());

    // Инициализация инвентаря
    if (!playerInventory.init(renderer, dialogFont, "assets/inventory/slot.png")) {
        SDL_Log("Ошибка инициализации инвентаря!");
    }

    // Инициализация меню паузы
    if (!pauseMenu.init(renderer, dialogFont)) {
        SDL_Log("Ошибка инициализации меню паузы!");
    }

    // Настройка колбэков для меню паузы
    pauseMenu.setVolumeCallback([&](float volume) {
        audioManager.setVolume(volume);
        });

    pauseMenu.setExitToMenuCallback([&]() {
        gameState = STATE_MENU;
        audioManager.stopMusic();
        audioManager.playMusic("assets/audio/Playboi-Carti-magnolia.wav", -1);
        });

    pauseMenu.setExitToDesktopCallback([&]() {
        SDL_Event quitEvent;
        quitEvent.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&quitEvent);
        });

    // Создание текстур UI
    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Surface* surf = TTF_RenderText_Blended(menuFont, "Press ENTER to start", 0, white);
    if (surf) {
        menuTextTexture = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_DestroySurface(surf);
    }

    surf = TTF_RenderText_Blended(menuFont, "Press ENTER to skip", 0, white);
    if (surf) {
        skipTextTexture = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_DestroySurface(surf);
    }
    else {
        SDL_Log("Ошибка создания skip-текстуры: %s", SDL_GetError());
    }

    // Загрузка фоновых текстур
    menuBackground = IMG_LoadTexture(renderer, "assets/images/enter.png");
    if (!menuBackground) SDL_Log("Фон меню не загружен: %s", SDL_GetError());

    disclaimerBG = IMG_LoadTexture(renderer, "assets/images/dis.png");
    if (!disclaimerBG) SDL_Log("Дисклеймер не загружен: %s", SDL_GetError());

    extraImage = IMG_LoadTexture(renderer, "assets/images/beg.png");
    if (!extraImage) SDL_Log("Доп. картинка не загружена: %s", SDL_GetError());

    // Создание NPC и игрока
    dialogueNPC = new NPC1(renderer, "assets/NPC/idle(64x64).png", 550.0f, 400.0f);
    dialogueNPC->dialogueLines = {
        "Привет! Ты попал на чудо остров!",
        "Хочешь, подарю тебе амулет?"
    };
    dialogueNPC->option1 = "Да(1)";
    dialogueNPC->option2 = "Нет(2)";

    player = new Player(renderer, "assets/player/split.png");
    player->worldX = 500.0f;
    player->worldY = 400.0f;

    // Загрузка карты
    tileMap = new TileMap();
    if (!tileMap->loadFromJSON("assets/map/1234.json")) {
        SDL_Log("Ошибка загрузки карты!");
        return SDL_APP_FAILURE;
    }
    if (!tileMap->loadTilesetTexture(renderer, "assets/Texture/TX Plant.png")) {
        SDL_Log("Ошибка загрузки тайлсета!");
    }
    if (tileMap) {
        player->setWorldBounds(
            tileMap->mapWidth * tileMap->tileWidth,
            tileMap->mapHeight * tileMap->tileHeight
        );
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    if (event->type == SDL_EVENT_QUIT)
        return SDL_APP_SUCCESS;

    // Обработка дисклеймера
    if (gameState == STATE_DISCLAIMER) {
        if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_RETURN)
            gameState = STATE_EXTRA;
        return SDL_APP_CONTINUE;
    }
    // Обработка экрана EXTRA
    else if (gameState == STATE_EXTRA) {
        if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_RETURN) {
            gameState = STATE_MENU;
            audioManager.playMusic("assets/audio/Playboi-Carti-magnolia.wav", -1);
        }
        return SDL_APP_CONTINUE;
    }
    // Обработка главного меню
    else if (gameState == STATE_MENU) {
        if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_RETURN) {
            gameState = STATE_GAME;
            audioManager.stopMusic();
            audioManager.playMusic("assets/audio/korolevskij_XVII_-_1_5__SkySound.cc__1.wav", -1);
            camera.zoom = 1.75f;
            camera.update(player->worldX + 32.0f, player->worldY + 64.0f,
                static_cast<float>(tileMap->mapWidth * tileMap->tileWidth),
                static_cast<float>(tileMap->mapHeight * tileMap->tileHeight));
        }
        return SDL_APP_CONTINUE;
    }
    // Обработка игры
    else if (gameState == STATE_GAME) {
        // Обработка ESC для открытия/закрытия меню паузы
        if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE) {
            if (pauseMenu.isOpen()) {
                pauseMenu.close();
            }
            else if (!dialogMgr.isActive()) {
                pauseMenu.open();
            }
            return SDL_APP_CONTINUE;
        }

        // Если меню паузы открыто - отправляем события в него
        if (pauseMenu.isOpen()) {
            pauseMenu.handleEvent(*event);
            return SDL_APP_CONTINUE;
        }

        // Остальная обработка игры (только когда меню закрыто)
        player->handleEvents();
        dialogueNPC->handleEvents();

        if (event->type == SDL_EVENT_KEY_DOWN) {
            // Если диалог в режиме выбора – реагируем на 1 и 2
            if (dialogMgr.isActive() && dialogMgr.isWaitingChoice()) {
                if (event->key.key == SDLK_1) {
                    dialogMgr.handleChoice(1);
                    audioManager.playMusic("assets/audio/korolevskij_XVII_-_1_5__SkySound.cc__1.wav", -1);

                    // Добавляем амулет в инвентарь (только один раз)
                    if (!amuletReceived) {
                        SDL_Texture* amuletIcon = IMG_LoadTexture(renderer, "assets/items/amulet.png");
                        if (amuletIcon) {
                            playerInventory.addItem("Magic Amulet", amuletIcon, 1);
                            SDL_Log("Вы получили магический амулет!");
                            amuletReceived = true;
                        }
                        else {
                            SDL_Log("Не удалось загрузить текстуру амулета: %s", SDL_GetError());
                        }
                    }
                }
                else if (event->key.key == SDLK_2) {
                    dialogMgr.handleChoice(2);
                    audioManager.playMusic("assets/audio/korolevskij_XVII_-_1_5__SkySound.cc__1.wav", -1);
                }
            }
            // Обычный диалог (E)
            else if (event->key.key == SDLK_E) {
                if (!dialogMgr.isActive()) {
                    if (dialogueNPC && dialogueNPC->isPlayerNear(player->worldX, player->worldY)) {
                        dialogMgr.show(
                            dialogueNPC->dialogueLines,
                            dialogueNPC->option1, [&]() { dialogueNPC->gaveItem = true; },
                            dialogueNPC->option2, [&]() { dialogueNPC->gaveItem = false; },
                            dialogFont, renderer
                        );
                        audioManager.playMusic("assets/audio/Playboi-Carti-Molly.wav", -1);
                    }
                }
                else {
                    dialogMgr.next();
                    if (!dialogMgr.isActive()) {
                        audioManager.playMusic("assets/audio/korolevskij_XVII_-_1_5__SkySound.cc__1.wav", -1);
                    }
                }
            }
            // Управление инвентарём (цифры 1-8)
            else if (event->key.key >= SDLK_1 && event->key.key <= SDLK_8) {
                int slot = event->key.key - SDLK_1;
                playerInventory.setSelectedSlot(slot);
            }
            // Использование предмета (клавиша F)
            else if (event->key.key == SDLK_F) {
                const InventorySlot* item = playerInventory.getSelectedItem();
                if (item) {
                    SDL_Log("Used item: %s x%d", item->itemName.c_str(), item->count);
                    if (item->itemName == "Magic Amulet") {
                        SDL_Log("You used the amulet! You feel magical power!");
                        playerInventory.removeItem(item->itemName, 1);
                    }
                }
            }
        }
        return SDL_APP_CONTINUE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    // Очистка экрана
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Обновление аудио каждый кадр (для зацикливания)
    audioManager.update();

    // Обновление меню паузы
    pauseMenu.update(0.016f);

    // Отрисовка в зависимости от состояния
    if (gameState == STATE_MENU) {
        if (menuBackground) {
            SDL_FRect dest = { 0, 0, 1920, 1080 };
            SDL_RenderTexture(renderer, menuBackground, NULL, &dest);
        }
        if (menuTextTexture) {
            float texW, texH;
            SDL_GetTextureSize(menuTextTexture, &texW, &texH);
            SDL_FRect dest = { 1920 / 2.0f - texW / 2.0f, 1080 - 150.0f, texW, texH };
            SDL_RenderTexture(renderer, menuTextTexture, NULL, &dest);
        }
    }
    else if (gameState == STATE_DISCLAIMER) {
        if (disclaimerBG) {
            SDL_FRect dest = { 0, 0, 1920, 1080 };
            SDL_RenderTexture(renderer, disclaimerBG, NULL, &dest);
        }
        if (skipTextTexture) {
            float texW, texH;
            SDL_GetTextureSize(skipTextTexture, &texW, &texH);
            SDL_FRect dest = { 1920 / 2.0f - texW / 2.0f, 1080 - 100.0f, texW, texH };
            SDL_RenderTexture(renderer, skipTextTexture, NULL, &dest);
        }
    }
    else if (gameState == STATE_EXTRA) {
        if (extraImage) {
            SDL_FRect dest = { 0, 0, 1920, 1080 };
            SDL_RenderTexture(renderer, extraImage, NULL, &dest);
        }
        if (skipTextTexture) {
            float texW, texH;
            SDL_GetTextureSize(skipTextTexture, &texW, &texH);
            SDL_FRect dest = { 1920 / 2.0f - texW / 2.0f, 1080 - 100.0f, texW, texH };
            SDL_RenderTexture(renderer, skipTextTexture, NULL, &dest);
        }
    }
    else if (gameState == STATE_GAME) {
        // Обновление камеры
        camera.update(player->worldX + 32.0f, player->worldY + 64.0f,
            static_cast<float>(tileMap->mapWidth * tileMap->tileWidth),
            static_cast<float>(tileMap->mapHeight * tileMap->tileHeight));

        // Отрисовка карты
        tileMap->render(renderer, camera.x, camera.y, camera.zoom);

        // Отрисовка NPC
        if (dialogueNPC) {
            dialogueNPC->update();
            dialogueNPC->draw(camera.x, camera.y, camera.zoom);
        }

        // Отрисовка игрока
        player->update();
        player->draw(camera.x, camera.y, camera.zoom);

        // Отрисовка диалога
        dialogMgr.draw(renderer);

        // Отрисовка меню паузы (если открыто)
        pauseMenu.draw(1920.0f, 1080.0f);
    }

    // Отрисовка инвентаря (всегда поверх всего, кроме меню паузы)
    if (gameState == STATE_GAME && !pauseMenu.isOpen()) {
        playerInventory.draw(1920.0f, 1080.0f);
    }

    SDL_RenderPresent(renderer);
    SDL_Delay(16);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    delete tileMap;

    // Очистка текстур UI
    if (menuBackground) SDL_DestroyTexture(menuBackground);
    if (menuTextTexture) SDL_DestroyTexture(menuTextTexture);
    if (disclaimerBG) SDL_DestroyTexture(disclaimerBG);
    if (extraImage) SDL_DestroyTexture(extraImage);
    if (skipTextTexture) SDL_DestroyTexture(skipTextTexture);

    // Очистка шрифтов
    if (menuFont) TTF_CloseFont(menuFont);
    if (dialogFont) TTF_CloseFont(dialogFont);

    // Очистка игровых объектов
    delete dialogueNPC;
    delete player;

    TTF_Quit();
    audioManager.cleanup();
    playerInventory.cleanup();
    pauseMenu.cleanup();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}