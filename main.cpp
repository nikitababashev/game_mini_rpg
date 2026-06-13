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

using json = nlohmann::json;

// Глобальные объекты
static Inventory playerInventory;
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

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    SDL_CreateWindowAndRenderer("beautiful_garden", 1920, 1080, 0, &window, &renderer);
    TTF_Init();

    // Инициализация аудио
    audioManager.init();

    menuFont = TTF_OpenFont("assets/Banty Bold.ttf", 72);
    if (!menuFont) SDL_Log("Ошибка загрузки menuFont: %s", SDL_GetError());

    dialogFont = TTF_OpenFont("assets/DejaVuSans.ttf", 48);
    if (!dialogFont) SDL_Log("Ошибка загрузки dialogFont: %s", SDL_GetError());
    if (!playerInventory.init(renderer, dialogFont, "assets/inventory/slot.png")) {
        SDL_Log("Ошибка инициализации инвентаря!");
    }

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

    menuBackground = IMG_LoadTexture(renderer, "assets/images/enter.png");
    if (!menuBackground) SDL_Log("Фон меню не загружен: %s", SDL_GetError());

    disclaimerBG = IMG_LoadTexture(renderer, "assets/images/dis.png");
    if (!disclaimerBG) SDL_Log("Дисклеймер не загружен: %s", SDL_GetError());

    extraImage = IMG_LoadTexture(renderer, "assets/images/beg.png");
    if (!extraImage) SDL_Log("Доп. картинка не загружена: %s", SDL_GetError());

    dialogueNPC = new NPC1(renderer, "assets/NPC/idle(64x64).png", 550.0f, 400.0f);
    dialogueNPC->dialogueLines = {
    "Привет! Ты попал на чудо остров!",
    "Хочешь, подарю тебе амулет?"
    };
    dialogueNPC->option1 = "Да(1)";
    dialogueNPC->option2 = "Нет(2)";
    player = new Player(renderer, "assets/player/split.png");

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

    if (gameState == STATE_DISCLAIMER) {
        if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_RETURN)
            gameState = STATE_EXTRA;
        return SDL_APP_CONTINUE;
    }
    else if (gameState == STATE_EXTRA) {
        if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_RETURN) {
            gameState = STATE_MENU;
            // Запускаем музыку главного меню

            audioManager.playMusic("assets/audio/Playboi-Carti-magnolia.wav", -1);
        }
        return SDL_APP_CONTINUE;
    }
    else if (gameState == STATE_MENU) {
        if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_RETURN) {
            gameState = STATE_GAME;
            audioManager.stopMusic();          // Останавливаем менюшную музыку
            // Запускаем игровую фоновую музыку (бесконечно)
            audioManager.playMusic("assets/audio/korolevskij_XVII_-_1_5__SkySound.cc__1.wav", -1);
            camera.zoom = 1.75f;
            camera.update(player->worldX + 32.0f, player->worldY + 64.0f,
                static_cast<float>(tileMap->mapWidth * tileMap->tileWidth),
                static_cast<float>(tileMap->mapHeight * tileMap->tileHeight));
        }
        return SDL_APP_CONTINUE;
    }
    else if (gameState == STATE_GAME) {
        player->handleEvents();
        dialogueNPC->handleEvents();

        if (event->type == SDL_EVENT_KEY_DOWN) {
            if (dialogMgr.isActive() && dialogMgr.isWaitingChoice()) {
                if (event->key.key == SDLK_1) {
                    dialogMgr.handleChoice(1);
                    audioManager.playMusic("assets/audio/korolevskij_XVII_-_1_5__SkySound.cc__1.wav", -1);
                }
                else if (event->key.key == SDLK_2) {
                    dialogMgr.handleChoice(2);
                    audioManager.playMusic("assets/audio/korolevskij_XVII_-_1_5__SkySound.cc__1.wav", -1);
                }
            }
            else if (event->key.key == SDLK_E) {
                if (!dialogMgr.isActive()) {
                    if (dialogueNPC && dialogueNPC->isPlayerNear(player->worldX, player->worldY)) {
                        dialogMgr.show(
                            dialogueNPC->dialogueLines,
                            dialogueNPC->option1,
                            [&]() {
                                dialogueNPC->gaveItem = true;
                                // ДОБАВЛЯЕМ АМУЛЕТ
                                SDL_Texture* amuletIcon = IMG_LoadTexture(renderer, "assets/items/amulet.png");
                                if (amuletIcon) {
                                    playerInventory.addItem("Magic Amulet", amuletIcon, 1);
                                    SDL_Log("Амулет добавлен в инвентарь!");
                                }
                            },
                            dialogueNPC->option2,
                            [&]() {
                                dialogueNPC->gaveItem = false;
                            },
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

            // Управление инвентарём
            else if (event->key.key >= SDLK_1 && event->key.key <= SDLK_8) {
                int slot = event->key.key - SDLK_1;
                playerInventory.setSelectedSlot(slot);
            }
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
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Обновление аудио каждый кадр (для зацикливания)
    audioManager.update();

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
        camera.update(player->worldX + 32.0f, player->worldY + 64.0f,
            static_cast<float>(tileMap->mapWidth * tileMap->tileWidth),
            static_cast<float>(tileMap->mapHeight * tileMap->tileHeight));
        tileMap->render(renderer, camera.x, camera.y, camera.zoom);

        if (dialogueNPC) {
            dialogueNPC->update();
            dialogueNPC->draw(camera.x, camera.y, camera.zoom);
        }

        player->update();
        player->draw(camera.x, camera.y, camera.zoom);

        dialogMgr.draw(renderer);
    }
    playerInventory.draw(1920.0f, 1080.0f);
    SDL_RenderPresent(renderer);
    SDL_Delay(16);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    delete tileMap;

    if (menuBackground) SDL_DestroyTexture(menuBackground);
    if (menuTextTexture) SDL_DestroyTexture(menuTextTexture);
    if (disclaimerBG) SDL_DestroyTexture(disclaimerBG);
    if (extraImage) SDL_DestroyTexture(extraImage);
    if (skipTextTexture) SDL_DestroyTexture(skipTextTexture);

    if (menuFont) TTF_CloseFont(menuFont);
    if (dialogFont) TTF_CloseFont(dialogFont);

    delete dialogueNPC;
    delete player;

    TTF_Quit();
    audioManager.cleanup();
    playerInventory.cleanup();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}