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
#include "CharacterSelect.h"

using json = nlohmann::json;

// Глобальные объекты
static Inventory playerInventory;
static PauseMenu pauseMenu;
static CharacterSelect characterSelect;
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

// Иконки предметов
SDL_Texture* amuletIcon = nullptr;
SDL_Texture* goldenFlowerIcon = nullptr;
SDL_Texture* magicCrystalIcon = nullptr;

// Текстуры для выбора пола
SDL_Texture* malePlayerTexture = nullptr;
SDL_Texture* femalePlayerTexture = nullptr;

// Квестовый предмет на карте
struct QuestItem {
    float x, y;
    float width = 32.0f;
    float height = 32.0f;
    bool collected = false;
    SDL_Texture* texture = nullptr;
    std::string name;
};

QuestItem goldenFlower;

// Флаги
bool amuletReceived = false;
bool waitingForCharacterSelect = false;
CharacterGender currentGender = CharacterGender::MALE;

// Вспомогательные функции для работы с инвентарём
bool addItemToInventory(const std::string& itemName, int count) {
    SDL_Texture* icon = nullptr;
    if (itemName == "Magic Amulet") icon = amuletIcon;
    else if (itemName == "Golden Flower") icon = goldenFlowerIcon;
    else if (itemName == "Magic Crystal") icon = magicCrystalIcon;

    if (icon) {
        return playerInventory.addItem(itemName, icon, count);
    }
    return false;
}

bool removeItemFromInventory(const std::string& itemName, int count) {
    bool result = playerInventory.removeItem(itemName, count);
    if (result) {
        SDL_Log("removeItemFromInventory: Removed %d x %s", count, itemName.c_str());
    }
    else {
        SDL_Log("removeItemFromInventory: Failed to remove %s", itemName.c_str());
    }
    return result;
}

bool hasItemInInventory(const std::string& itemName) {
    return playerInventory.hasItem(itemName);
}

// Создание текстуры-заглушки
SDL_Texture* createPlaceholderTexture(SDL_Color color) {
    SDL_Surface* surf = SDL_CreateSurface(32, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!surf) return nullptr;
    SDL_FillSurfaceRect(surf, nullptr, SDL_MapSurfaceRGBA(surf, color.r, color.g, color.b, color.a));
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_DestroySurface(surf);
    return texture;
}

// Функция обработки диалога с NPC
void handleNPCDialogue() {
    if (!dialogueNPC || !dialogueNPC->isPlayerNear(player->worldX, player->worldY)) return;

    if (dialogMgr.isActive()) return;

    bool hasAmuletItem = hasItemInInventory("Magic Amulet");
    bool hasFlowerItem = hasItemInInventory("Golden Flower");

    if (dialogueNPC->questCompleted) {
        dialogMgr.show(
            { "Thanks for your help! Come again!" },
            "", nullptr, "", nullptr,
            dialogFont, renderer
        );
        return;
    }

    if (dialogueNPC->questRewarded) {
        dialogMgr.show(
            { "Thank you! The magic crystal will help you on your journey!" },
            "", nullptr, "", nullptr,
            dialogFont, renderer
        );
        dialogueNPC->questRewarded = false;
        dialogueNPC->questCompleted = true;
        return;
    }

    if (dialogueNPC->questActive && hasFlowerItem) {
        dialogMgr.show(
            dialogueNPC->questCompleteLines,
            "Complete Quest(1)", [&]() {
                removeItemFromInventory("Golden Flower", 1);
                addItemToInventory("Magic Crystal", 1);
                dialogueNPC->questRewarded = true;
                dialogueNPC->questActive = false;
                SDL_Log("Quest completed! Magic Crystal received!");
            },
            "Later(2)", []() {},
            dialogFont, renderer
        );
        return;
    }

    if (dialogueNPC->questActive && !hasFlowerItem) {
        dialogMgr.show(
            dialogueNPC->questProgressLines,
            "", nullptr, "", nullptr,
            dialogFont, renderer
        );
        return;
    }

    if (hasAmuletItem && !dialogueNPC->questActive && !dialogueNPC->questCompleted && !dialogueNPC->questRewarded) {
        dialogMgr.show(
            dialogueNPC->questStartLines,
            dialogueNPC->questOption1, [&]() {
                dialogueNPC->questActive = true;
                SDL_Log("Quest started! Find the Golden Flower!");
            },
            dialogueNPC->questOption2, []() {},
            dialogFont, renderer
        );
        return;
    }

    if (!hasAmuletItem && !amuletReceived) {
        dialogMgr.show(
            dialogueNPC->dialogueLines,
            dialogueNPC->option1, [&]() {
                addItemToInventory("Magic Amulet", 1);
                amuletReceived = true;
                SDL_Log("Amulet received!");
            },
            dialogueNPC->option2, []() {},
            dialogFont, renderer
        );
        return;
    }

    dialogMgr.show(
        { "Be careful in the forest, traveler!" },
        "", nullptr, "", nullptr,
        dialogFont, renderer
    );
}

// Инициализация квестового предмета
void initQuestItem() {
    goldenFlower.x = 1200.0f;
    goldenFlower.y = 800.0f;
    goldenFlower.width = 32.0f;
    goldenFlower.height = 32.0f;
    goldenFlower.collected = false;
    goldenFlower.name = "Golden Flower";

    goldenFlower.texture = IMG_LoadTexture(renderer, "assets/items/golden_flower.png");
    if (!goldenFlower.texture) {
        goldenFlower.texture = createPlaceholderTexture({ 255, 215, 0, 255 });
    }
}

// Отрисовка квестового предмета
void drawQuestItem(float camX, float camY, float zoom) {
    if (goldenFlower.collected) return;
    if (!goldenFlower.texture) return;

    float screenX = (goldenFlower.x - camX) * zoom;
    float screenY = (goldenFlower.y - camY) * zoom;
    float screenW = goldenFlower.width * zoom;
    float screenH = goldenFlower.height * zoom;

    static float pulse = 0.0f;
    pulse += 0.05f;
    float scale = 1.0f + sin(pulse) * 0.1f;

    SDL_FRect dest = {
        screenX - (goldenFlower.width * (scale - 1.0f) / 2.0f),
        screenY - (goldenFlower.height * (scale - 1.0f) / 2.0f),
        screenW * scale,
        screenH * scale
    };

    SDL_RenderTexture(renderer, goldenFlower.texture, nullptr, &dest);
}

// Проверка подбора предмета
void checkQuestItemPickup() {
    if (goldenFlower.collected) return;
    if (!dialogueNPC->questActive) return;

    float playerCenterX = player->worldX + 32.0f;
    float playerCenterY = player->worldY + 64.0f;
    float itemCenterX = goldenFlower.x + goldenFlower.width / 2.0f;
    float itemCenterY = goldenFlower.y + goldenFlower.height / 2.0f;

    float dx = playerCenterX - itemCenterX;
    float dy = playerCenterY - itemCenterY;
    float dist = (float)sqrt(dx * dx + dy * dy);

    if (dist < 50.0f) {
        goldenFlower.collected = true;
        addItemToInventory("Golden Flower", 1);
        SDL_Log("You found the Golden Flower!");

        dialogMgr.show(
            { "You found the Golden Flower! Return to the NPC." },
            "", nullptr, "", nullptr,
            dialogFont, renderer
        );
    }
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    SDL_CreateWindowAndRenderer("beautiful_garden", 1920, 1080, 0, &window, &renderer);
    TTF_Init();

    audioManager.init();

    menuFont = TTF_OpenFont("assets/Banty Bold.ttf", 72);
    dialogFont = TTF_OpenFont("assets/DejaVuSans.ttf", 48);

    playerInventory.init(renderer, dialogFont, "assets/inventory/slot.png");
    pauseMenu.init(renderer, dialogFont);
    pauseMenu.setWindow(window);

    // Загрузка иконок
    amuletIcon = IMG_LoadTexture(renderer, "assets/items/amulet.png");
    if (!amuletIcon) amuletIcon = createPlaceholderTexture({ 100, 100, 255, 255 });

    goldenFlowerIcon = IMG_LoadTexture(renderer, "assets/items/golden_flower.png");
    if (!goldenFlowerIcon) goldenFlowerIcon = createPlaceholderTexture({ 255, 215, 0, 255 });

    magicCrystalIcon = IMG_LoadTexture(renderer, "assets/items/magic_crystal.png");
    if (!magicCrystalIcon) magicCrystalIcon = createPlaceholderTexture({ 100, 150, 255, 255 });

    // Загрузка текстур для выбора пола
    malePlayerTexture = IMG_LoadTexture(renderer, "assets/player/male.png");
    if (!malePlayerTexture) {
        SDL_Surface* surf = SDL_CreateSurface(200, 200, SDL_PIXELFORMAT_ARGB8888);
        SDL_FillSurfaceRect(surf, nullptr, SDL_MapSurfaceRGBA(surf, 100, 150, 255, 255));
        malePlayerTexture = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_DestroySurface(surf);
        SDL_Log("Created placeholder for male player texture");
    }

    femalePlayerTexture = IMG_LoadTexture(renderer, "assets/player/female.png");
    if (!femalePlayerTexture) {
        SDL_Surface* surf = SDL_CreateSurface(200, 200, SDL_PIXELFORMAT_ARGB8888);
        SDL_FillSurfaceRect(surf, nullptr, SDL_MapSurfaceRGBA(surf, 255, 150, 200, 255));
        femalePlayerTexture = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_DestroySurface(surf);
        SDL_Log("Created placeholder for female player texture");
    }

    // Настройка колбэков для меню паузы
    pauseMenu.setVolumeCallback([&](float volume) { audioManager.setVolume(volume); });
    pauseMenu.setExitToMenuCallback([&]() {
        gameState = STATE_MENU;
        waitingForCharacterSelect = false;
        characterSelect.close();
        audioManager.stopMusic();
        audioManager.playMusic("assets/audio/Playboi-Carti-magnolia.wav", -1);
        SDL_Log("Exit to menu");
        });
    pauseMenu.setExitToDesktopCallback([&]() {
        SDL_Event quitEvent;
        quitEvent.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&quitEvent);
        });

    // Инициализация окна выбора персонажа
    characterSelect.init(renderer, window, dialogFont, malePlayerTexture, femalePlayerTexture);
    characterSelect.setOnConfirmCallback([&](CharacterGender gender) {
        SDL_Log("=== CONFIRM CALLBACK TRIGGERED ===");
        currentGender = gender;
        waitingForCharacterSelect = false;
        characterSelect.close();

        if (gender == CharacterGender::MALE) {
            player->setGenderTexture(malePlayerTexture);
            SDL_Log("Selected: MALE");
        }
        else {
            player->setGenderTexture(femalePlayerTexture);
            SDL_Log("Selected: FEMALE");
        }

        gameState = STATE_GAME;

        camera.zoom = 1.75f;
        camera.update(player->worldX + 32.0f, player->worldY + 64.0f,
            (float)(tileMap->mapWidth * tileMap->tileWidth),
            (float)(tileMap->mapHeight * tileMap->tileHeight));

        audioManager.setVolume(1.0f);
        SDL_Log("Game started! State changed to GAME");
        });

    // Текстуры UI
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

    menuBackground = IMG_LoadTexture(renderer, "assets/images/enter.png");
    disclaimerBG = IMG_LoadTexture(renderer, "assets/images/dis.png");
    extraImage = IMG_LoadTexture(renderer, "assets/images/beg.png");

    // Создание NPC и игрока
    dialogueNPC = new NPC1(renderer, "assets/NPC/idle(64x64).png", 550.0f, 400.0f);
    dialogueNPC->setHasItemCallback(hasItemInInventory);
    dialogueNPC->setRemoveItemCallback(removeItemFromInventory);
    dialogueNPC->setAddItemCallback(addItemToInventory);

    player = new Player(renderer, "assets/player/male.png");
    player->worldX = 500.0f;
    player->worldY = 400.0f;

    // Загрузка карты
    tileMap = new TileMap();
    tileMap->loadFromJSON("assets/map/1234.json");
    tileMap->loadTilesetTexture(renderer, "assets/Texture/TX Plant.png");
    player->setWorldBounds(
        (float)(tileMap->mapWidth * tileMap->tileWidth),
        (float)(tileMap->mapHeight * tileMap->tileHeight)
    );

    initQuestItem();

    gameState = STATE_MENU;
    SDL_Log("Game initialized, state = MENU");

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    if (event->type == SDL_EVENT_QUIT)
        return SDL_APP_SUCCESS;

    if (gameState == STATE_DISCLAIMER) {
        if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_RETURN) {
            gameState = STATE_EXTRA;
            SDL_Log("Transition: DISCLAIMER -> EXTRA");
        }
        return SDL_APP_CONTINUE;
    }
    else if (gameState == STATE_EXTRA) {
        if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_RETURN) {
            gameState = STATE_MENU;
            audioManager.playMusic("assets/audio/Playboi-Carti-magnolia.wav", -1);
            SDL_Log("Transition: EXTRA -> MENU");
        }
        return SDL_APP_CONTINUE;
    }
    else if (gameState == STATE_MENU) {
        // Если ожидаем выбора персонажа - передаём события ему
        if (waitingForCharacterSelect) {
            characterSelect.handleEvent(*event);
            return SDL_APP_CONTINUE;
        }

        if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_RETURN) {
            SDL_Log("ENTER pressed in menu, opening character select");
            waitingForCharacterSelect = true;
            characterSelect.open();
            audioManager.stopMusic();
            audioManager.playMusic("assets/audio/korolevskij_XVII_-_1_5__SkySound.cc__1.wav", -1);
            audioManager.setVolume(0.5f);
        }
        return SDL_APP_CONTINUE;
    }
    else if (gameState == STATE_GAME) {
        if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE) {
            if (pauseMenu.isOpen()) {
                pauseMenu.close();
                SDL_Log("Pause menu closed");
            }
            else if (!dialogMgr.isActive()) {
                pauseMenu.open();
                SDL_Log("Pause menu opened");
            }
            return SDL_APP_CONTINUE;
        }

        if (pauseMenu.isOpen()) {
            pauseMenu.handleEvent(*event);
            return SDL_APP_CONTINUE;
        }

        player->handleEvents();

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
                    handleNPCDialogue();
                }
                else {
                    dialogMgr.next();
                    if (!dialogMgr.isActive()) {
                        audioManager.playMusic("assets/audio/korolevskij_XVII_-_1_5__SkySound.cc__1.wav", -1);
                    }
                }
            }
            else if (event->key.key >= SDLK_1 && event->key.key <= SDLK_8) {
                int slot = event->key.key - SDLK_1;
                playerInventory.setSelectedSlot(slot);
                SDL_Log("Selected inventory slot: %d", slot);
            }
            else if (event->key.key == SDLK_F) {
                const InventorySlot* item = playerInventory.getSelectedItem();
                if (item && !item->isEmpty()) {
                    SDL_Log("=== Using item: %s (count: %d) ===", item->itemName.c_str(), item->count);

                    if (item->itemName == "Magic Crystal") {
                        SDL_Log("You used the magic crystal! It shines brightly!");
                        playerInventory.removeItem("Magic Crystal", 1);
                    }
                    else if (item->itemName == "Golden Flower") {
                        SDL_Log("You used the Golden Flower! It scatters petals!");
                        playerInventory.removeItem("Golden Flower", 1);
                    }
                    else if (item->itemName == "Magic Amulet") {
                        SDL_Log("You used the amulet! You feel magical power!");
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

    audioManager.update();
    pauseMenu.update(0.016f);

    if (gameState == STATE_MENU) {
        if (waitingForCharacterSelect) {
            if (menuBackground) {
                SDL_FRect dest = { 0, 0, 1920, 1080 };
                SDL_RenderTexture(renderer, menuBackground, NULL, &dest);
            }
            characterSelect.draw();
        }
        else {
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
            (float)(tileMap->mapWidth * tileMap->tileWidth),
            (float)(tileMap->mapHeight * tileMap->tileHeight));

        tileMap->render(renderer, camera.x, camera.y, camera.zoom);
        drawQuestItem(camera.x, camera.y, camera.zoom);
        checkQuestItemPickup();

        if (dialogueNPC) {
            dialogueNPC->update();
            dialogueNPC->draw(camera.x, camera.y, camera.zoom);
        }

        player->update();
        player->draw(camera.x, camera.y, camera.zoom);
        dialogMgr.draw(renderer);
        pauseMenu.draw(1920.0f, 1080.0f);

        if (!pauseMenu.isOpen()) {
            playerInventory.draw(1920.0f, 1080.0f);
        }
    }

    SDL_RenderPresent(renderer);
    SDL_Delay(16);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    SDL_Log("Application quitting...");

    delete tileMap;

    if (menuBackground) SDL_DestroyTexture(menuBackground);
    if (menuTextTexture) SDL_DestroyTexture(menuTextTexture);
    if (disclaimerBG) SDL_DestroyTexture(disclaimerBG);
    if (extraImage) SDL_DestroyTexture(extraImage);
    if (skipTextTexture) SDL_DestroyTexture(skipTextTexture);

    if (amuletIcon) SDL_DestroyTexture(amuletIcon);
    if (goldenFlowerIcon) SDL_DestroyTexture(goldenFlowerIcon);
    if (magicCrystalIcon) SDL_DestroyTexture(magicCrystalIcon);
    if (goldenFlower.texture && goldenFlower.texture != goldenFlowerIcon) {
        SDL_DestroyTexture(goldenFlower.texture);
    }

    if (malePlayerTexture) SDL_DestroyTexture(malePlayerTexture);
    if (femalePlayerTexture) SDL_DestroyTexture(femalePlayerTexture);

    if (menuFont) TTF_CloseFont(menuFont);
    if (dialogFont) TTF_CloseFont(dialogFont);

    delete dialogueNPC;
    delete player;

    TTF_Quit();
    audioManager.cleanup();
    playerInventory.cleanup();
    pauseMenu.cleanup();
    characterSelect.cleanup();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    SDL_Log("Application quit successfully");
}