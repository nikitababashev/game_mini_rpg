#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
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
TTF_Font* cutsceneFont = nullptr;
SDL_Texture* menuTextTexture = nullptr;
SDL_Texture* disclaimerBG = nullptr;
SDL_Texture* extraImage = nullptr;
SDL_Texture* skipTextTexture = nullptr;

NPC1* dialogueNPC = nullptr;

// Иконки предметов
SDL_Texture* amuletIcon = nullptr;
SDL_Texture* goldenFlowerIcon = nullptr;
SDL_Texture* magicCrystalIcon = nullptr;

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

// -------- Катсцены (между актами) --------
std::vector<std::string> cutsceneLines;
size_t cutsceneLineIndex = 0;
SDL_Texture* fullLineTexture = nullptr;      // полная текстура текущей строки
std::string lastLine;                        // последняя отрисованная строка (для отслеживания смены)
float cutsceneCharTimer = 0.0f;
int cutsceneVisibleChars = 0;
const float CUTSCENE_CHAR_SPEED = 60.0f;     // символов в секунду

void startCutscene(const std::vector<std::string>& lines) {
    cutsceneLines = lines;
    cutsceneLineIndex = 0;
    cutsceneCharTimer = 0.0f;
    cutsceneVisibleChars = 0;
    // Останавливаем игровую музыку, если нужно
    audioManager.stopMusic();
    // Можно запустить музыку катсцены: audioManager.playMusic("...", -1);
    gameState = STATE_CUTSCENE;
}

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
    return playerInventory.removeItem(itemName, count);
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

    if (dialogueNPC->questRewarded) {
        dialogMgr.show(
            { "Спасибо! Магический кристалл поможет тебе в пути!" },
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
            "Сдать квест(1)", [&]() {
                removeItemFromInventory("Golden Flower", 1);
                addItemToInventory("Magic Crystal", 1);
                dialogueNPC->questRewarded = true;
                dialogueNPC->questActive = false;
            },
            "Позже(2)", []() {},
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
            dialogueNPC->questOption1, [&]() { dialogueNPC->questActive = true; },
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
            },
            dialogueNPC->option2, []() {},
            dialogFont, renderer
        );
        return;
    }

    dialogMgr.show(
        { "Будь осторожен в лесу, путник!" },
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
    float dist = sqrt(dx * dx + dy * dy);

    if (dist < 50.0f) {
        goldenFlower.collected = true;
        addItemToInventory("Golden Flower", 1);
        dialogMgr.show(
            { "Вы нашли Золотой Цветок!" },
            "", nullptr, "", nullptr,
            dialogFont, renderer
        );
    }
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    SDL_CreateWindowAndRenderer("MORVEIN", 1920, 1080, 0, &window, &renderer);
    TTF_Init();

    audioManager.init();

    menuFont = TTF_OpenFont("assets/Banty Bold.ttf", 72);
    dialogFont = TTF_OpenFont("assets/DejaVuSans.ttf", 48);
    cutsceneFont = TTF_OpenFont("assets/DejaVuSans.ttf", 56); // для катсцен

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

    pauseMenu.setVolumeCallback([&](float volume) { audioManager.setVolume(volume); });
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

    player = new Player(renderer, "assets/player/1.png");
    player->worldX = 500.0f;
    player->worldY = 400.0f;

    // Загрузка карты
    tileMap = new TileMap();
    tileMap->loadFromJSON("assets/map/1234.json");
    tileMap->loadTilesetTexture(renderer, "assets/Texture/TX Plant.png");
    player->setWorldBounds(
        tileMap->mapWidth * tileMap->tileWidth,
        tileMap->mapHeight * tileMap->tileHeight
    );

    initQuestItem();

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
            audioManager.playMusic("assets/audio/Playboi-Carti-magnolia.wav", -1);
        }
        return SDL_APP_CONTINUE;
    }
    else if (gameState == STATE_MENU) {
        if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_RETURN) {
            // Вместо прямого запуска игры начинаем катсцену
            startCutscene({
                "ЧАСТЬ 1. ЭПОХА ДО БЕЗДНЫ",
                "За тысячи лет до событий игры мир был другим.",
                "Не существовало королевств, людей было мало.", "Большая часть земель принадлежала Первородным.",
                "Они были бессмертны. Не старели. Не болели.",
                "Люди просыли младшей расой и жили под их контролем.",
                "Мир существовал в равновесии.Но это продолжалось недолго."
                });
        }
        return SDL_APP_CONTINUE;
    }
    else if (gameState == STATE_CUTSCENE) {
        if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_RETURN) {
            // Если текст ещё печатается — сразу показать всю строку
            if (!cutsceneLines.empty() && cutsceneLineIndex < cutsceneLines.size()) {
                std::string& line = cutsceneLines[cutsceneLineIndex];
                if (cutsceneVisibleChars < (int)line.length()) {
                    cutsceneVisibleChars = (int)line.length();
                    cutsceneCharTimer = (float)line.length() / CUTSCENE_CHAR_SPEED;
                }
                else {
                    // Переход к следующей строке
                    cutsceneLineIndex++;
                    cutsceneVisibleChars = 0;
                    cutsceneCharTimer = 0.0f;
                    // Очищаем текстуру для следующей строки
                    if (fullLineTexture) {
                        SDL_DestroyTexture(fullLineTexture);
                        fullLineTexture = nullptr;
                    }
                    lastLine.clear();

                    // Если это была последняя строка — завершаем катсцену и запускаем игру
                    if (cutsceneLineIndex >= cutsceneLines.size()) {
                        gameState = STATE_GAME;
                        audioManager.stopMusic();
                        audioManager.playMusic("assets/audio/korolevskij_XVII_-_1_5__SkySound.cc__1.wav", -1);
                        camera.zoom = 1.75f;
                        camera.update(player->worldX + 32.0f, player->worldY + 64.0f,
                            (float)(tileMap->mapWidth * tileMap->tileWidth),
                            (float)(tileMap->mapHeight * tileMap->tileHeight));
                        if (fullLineTexture) { SDL_DestroyTexture(fullLineTexture); fullLineTexture = nullptr; }
                        lastLine.clear();
                    }
                }
            }
        }
        return SDL_APP_CONTINUE;
    }
    else if (gameState == STATE_GAME) {
        // ESC для меню паузы
        if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE) {
            if (pauseMenu.isOpen()) {
                pauseMenu.close();
            }
            else if (!dialogMgr.isActive()) {
                pauseMenu.open();
            }
            return SDL_APP_CONTINUE;
        }

        if (pauseMenu.isOpen()) {
            pauseMenu.handleEvent(*event);
            return SDL_APP_CONTINUE;
        }

        player->handleEvents();

        if (event->type == SDL_EVENT_KEY_DOWN) {
            // Выбор в диалоге
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
            // Диалог (E)
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
            // Инвентарь (1-8)
            else if (event->key.key >= SDLK_1 && event->key.key <= SDLK_8) {
                int slot = event->key.key - SDLK_1;
                playerInventory.setSelectedSlot(slot);
            }
            // Использование предмета (F)
            else if (event->key.key == SDLK_F) {
                const InventorySlot* item = playerInventory.getSelectedItem();
                if (item && !item->isEmpty()) {
                    if (item->itemName == "Magic Crystal") {
                        if (playerInventory.removeItem("Magic Crystal", 1)) {
                            SDL_Log("SUCCESS: Magic Crystal used!");
                        }
                    }
                    else if (item->itemName == "Golden Flower") {
                        SDL_Log("Used the flower, but it stays.");
                    }
                    else if (item->itemName == "Magic Amulet") {
                        SDL_Log("Amulet used, but remains.");
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

    static Uint64 lastTicks = SDL_GetTicks();
    Uint64 nowTicks = SDL_GetTicks();
    float deltaTime = (nowTicks - lastTicks) / 1000.0f;
    lastTicks = nowTicks;

    audioManager.update();
    pauseMenu.update(0.016f);

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
    else if (gameState == STATE_CUTSCENE) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (!cutsceneLines.empty() && cutsceneLineIndex < cutsceneLines.size()) {
            std::string& line = cutsceneLines[cutsceneLineIndex];

            // Плавное накопление времени
            cutsceneCharTimer += deltaTime;
            int targetChars = (int)(cutsceneCharTimer * CUTSCENE_CHAR_SPEED);
            if (targetChars > (int)line.length()) targetChars = (int)line.length();
            cutsceneVisibleChars = targetChars;

            // Создаём полную текстуру строки, если она изменилась
            if (line != lastLine || !fullLineTexture) {
                if (fullLineTexture) SDL_DestroyTexture(fullLineTexture);
                fullLineTexture = nullptr;

                SDL_Color white = { 255, 255, 255, 255 };
                SDL_Surface* surf = TTF_RenderText_Blended(cutsceneFont, line.c_str(), 0, white);
                if (surf) {
                    fullLineTexture = SDL_CreateTextureFromSurface(renderer, surf);
                    SDL_DestroySurface(surf);
                    lastLine = line;
                }
            }

            if (fullLineTexture) {
                // Получаем размер всей текстуры
                float fullTexW, fullTexH;
                SDL_GetTextureSize(fullLineTexture, &fullTexW, &fullTexH);

                // Определяем ширину видимой части в зависимости от количества отображаемых символов
                float visibleWidth = 0.0f;
                if (cutsceneVisibleChars > 0) {
                    visibleWidth = fullTexW * ((float)cutsceneVisibleChars / line.length());
                }

                if (visibleWidth > fullTexW) visibleWidth = fullTexW;

                // Source rect - вырезаем левую часть текстуры от 0 до visibleWidth
                SDL_FRect srcRect = { 0.0f, 0.0f, visibleWidth, fullTexH };

                // Destination rect - позиция по центру экрана
                float destX = 1920.0f / 2.0f - fullTexW / 2.0f;
                float destY = 1080.0f / 2.0f - fullTexH / 2.0f;
                SDL_FRect destRect = { destX, destY, visibleWidth, fullTexH };

                SDL_RenderTexture(renderer, fullLineTexture, &srcRect, &destRect);
            }
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
    }

    if (gameState == STATE_GAME && !pauseMenu.isOpen()) {
        playerInventory.draw(1920.0f, 1080.0f);
    }

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

    if (amuletIcon) SDL_DestroyTexture(amuletIcon);
    if (goldenFlowerIcon) SDL_DestroyTexture(goldenFlowerIcon);
    if (magicCrystalIcon) SDL_DestroyTexture(magicCrystalIcon);
    if (goldenFlower.texture && goldenFlower.texture != goldenFlowerIcon) {
        SDL_DestroyTexture(goldenFlower.texture);
    }

    if (fullLineTexture) SDL_DestroyTexture(fullLineTexture);

    if (menuFont) TTF_CloseFont(menuFont);
    if (dialogFont) TTF_CloseFont(dialogFont);
    if (cutsceneFont) TTF_CloseFont(cutsceneFont);

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