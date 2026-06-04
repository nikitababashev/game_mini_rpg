#define SDL_MAIN_USE_CALLBACKS 1
#include<SDL3/SDL.h>
#include<SDL3/SDL_main.h>
#include<iostream>
#include<stdio.h>
#include<SDL3_image/SDL_image.h>
#include "Player.h"
#include "NPC.h"
#include "Resource.h"
#include <fstream>          // для чтения файла
#include "json.hpp"         // JSON-библиотека (nlohmann)
#include <vector>           // уже может быть подключён, не повредит
#include<SDL3_ttf/SDL_ttf.h>
#include "GameState.h"
#include "TileMap.h"
#include "NPC1.h"
#include "Camera.h"
#include "DialogManager.h"
DialogManager dialogMgr;
Camera camera;
static SDL_Renderer* renderer;
using json = nlohmann::json;

static SDL_Window* window;

Player* player = nullptr;
NPC* npc = nullptr;
Resource* thing = nullptr;
TileMap* tileMap = nullptr;
SDL_Texture* menuBackground = nullptr;
TTF_Font* menuFont = nullptr;
TTF_Font* dialogFont = nullptr;
SDL_Texture* menuTextTexture = nullptr;
SDL_Texture* disclaimerBG = nullptr;
SDL_Texture* extraImage = nullptr;
SDL_Texture* skipTextTexture = nullptr;
NPC1* dialogueNPC = nullptr;   // наш новый интерактивный NPC

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	SDL_Init(SDL_INIT_VIDEO);//видео система авт. инициал ивенты подсистемы
	SDL_CreateWindowAndRenderer("beautiful_garden", 1920, 1080, 0, &window, &renderer);
    TTF_Init();
    menuFont = TTF_OpenFont("assets/Banty Bold.ttf", 72);   // размер 72 подбери сам
    if (!menuFont) {
        SDL_Log("Ошибка загрузки шрифта: %s", SDL_GetError());
    }
    dialogFont = TTF_OpenFont("assets/DejaVuSans.ttf", 48); // размер подбери под диалоги
    if (!dialogFont) {
        SDL_Log("Ошибка загрузки шрифта диалогов: %s", SDL_GetError());
    }
    SDL_Color textColor = { 255, 255, 255, 255 };   // белый цвет
    SDL_Surface* textSurface = TTF_RenderText_Blended(menuFont, "Press ENTER to start", 0, textColor);
    if (textSurface) {
        menuTextTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_DestroySurface(textSurface);   // поверхность больше не нужна
    }
    SDL_Color skipColor = { 255, 255, 255, 255 };   // белый цвет
    SDL_Surface* skipSurface = TTF_RenderText_Blended(menuFont, "Press ENTER to skip", 0, skipColor);
    if (skipSurface) {
        skipTextTexture = SDL_CreateTextureFromSurface(renderer, skipSurface);
        SDL_DestroySurface(skipSurface);
    }
    else {
        SDL_Log("Ошибка создания skip-текстуры: %s", SDL_GetError());
    }

    menuBackground = IMG_LoadTexture(renderer, "assets/images/enter.png");
    if (!menuBackground) {
        SDL_Log("Не загрузился фон меню: %s", SDL_GetError());
    }

    disclaimerBG = IMG_LoadTexture(renderer, "assets/images/dis.png");
    if (!disclaimerBG) {
        SDL_Log("Не загрузился дисклеймер: %s", SDL_GetError());
    }
    extraImage = IMG_LoadTexture(renderer, "assets/images/beg.png");
    if (!extraImage) {
        SDL_Log("Не загрузилась доп. картинка: %s", SDL_GetError());
    }
    // Размещаем NPC в точке (800, 600) мира (можно привязать к тайлам)
    dialogueNPC = new NPC1(renderer, "assets/NPC/idle(64x64).png", 550.0f, 400.0f);
	player = new Player(renderer, "assets/player/split.png");
	npc = new NPC(renderer, "assets/NPC/idle(64x64).png");
	/*thing = new Resource();*/
    tileMap = new TileMap();
    if (!tileMap->loadFromJSON("assets/map/1234.json")) {
        SDL_Log("Ошибка загрузки карты!");
        return SDL_APP_FAILURE;   // или продолжить без карты, как хочешь
    }
    if (!tileMap->loadTilesetTexture(renderer, "assets/Texture/TX Plant.png")) {
        SDL_Log("Ошибка загрузки текстуры тайлсета!");
        // Не обязательно падать, можно рисовать без текстуры
    }
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    if (gameState == STATE_DISCLAIMER) {
        if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_RETURN) {
            // Переходим на дисклеймер
            gameState = STATE_EXTRA;
        }
        return SDL_APP_CONTINUE;
    }
    else if (gameState == STATE_EXTRA) {
        if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_RETURN) {
            gameState = STATE_MENU;   // идём на доп. картинку
        }
        return SDL_APP_CONTINUE;
    }
    else if (gameState == STATE_MENU) {
        if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_RETURN) {
            gameState = STATE_GAME;
            // Ничего не нужно – камера обновится в следующем кадре.
// Но чтобы не было рывка, можно принудительно вызвать update с начальным приближением:
            camera.zoom = 1.75f;   // или сохранить текущий зум
            camera.update(player->worldX + 32.0f, player->worldY + 64.0f,
                static_cast<float>(tileMap->mapWidth * tileMap->tileWidth),
                static_cast<float>(tileMap->mapHeight * tileMap->tileHeight));
        }
        return SDL_APP_CONTINUE;
    }
    else if (gameState == STATE_GAME) {
        player->handleEvents();
        npc->handleEvents();   // старый NPC, если он что-то делает

        if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_E) {
            if (!dialogMgr.isActive() && dialogueNPC && dialogueNPC->isPlayerNear(player->worldX, player->worldY)) {
                dialogMgr.show(dialogueNPC->dialogueText, dialogFont, renderer);
            }
            else if (dialogMgr.isActive()) {
                dialogMgr.hide();
            }
        }
    
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (gameState == STATE_MENU) {
        // Рисуем твой фон
        if (menuBackground) {
            SDL_FRect dest = { 0, 0, 1920, 1080 };
            SDL_RenderTexture(renderer, menuBackground, NULL, &dest);
        }
        if (menuTextTexture) {
            // Размести надпись в центре экрана или где хочешь
            float texW, texH;
            SDL_GetTextureSize(menuTextTexture, &texW, &texH);
            SDL_FRect dest = {
                1920 / 2.0f - texW / 2.0f,   // по центру по X
                1080 - 150.0f,               // отступ снизу
                texW, texH
            };
            SDL_RenderTexture(renderer, menuTextTexture, NULL, &dest);
        }
    }
    else if (gameState == STATE_DISCLAIMER) {
        // Показываем картинку дисклеймера
        if (disclaimerBG) {
            SDL_FRect dest = { 0, 0, 1920, 1080 };   // под размер окна
            SDL_RenderTexture(renderer, disclaimerBG, NULL, &dest);
        }
        if (skipTextTexture) {
            float texW, texH;
            SDL_GetTextureSize(skipTextTexture, &texW, &texH);
            SDL_FRect dest = {
                1920 / 2.0f - texW / 2.0f,   // центр по X
                1080 - 100.0f,               // отступ снизу 100px
                texW, texH
            };
            SDL_RenderTexture(renderer, skipTextTexture, NULL, &dest);
        }
    }
        // По желанию можно добавить маленькую подсказку "Нажмите ENTER"
        else if (gameState == STATE_EXTRA) {
            if (extraImage) {
                SDL_FRect dest = { 0, 0, 1920, 1080 };
                SDL_RenderTexture(renderer, extraImage, NULL, &dest);
            }
            // По желанию — подсказка "Нажмите ENTER для продолжения"
            if (skipTextTexture) {
                float texW, texH;
                SDL_GetTextureSize(skipTextTexture, &texW, &texH);
                SDL_FRect dest = {
                    1920 / 2.0f - texW / 2.0f,   // центр по X
                    1080 - 100.0f,               // отступ снизу 100px
                    texW, texH
                };
                SDL_RenderTexture(renderer, skipTextTexture, NULL, &dest);
            }
    }
    else if (gameState == STATE_GAME) {
        camera.update(player->worldX + 32.0f, player->worldY + 64.0f,
            static_cast<float>(tileMap->mapWidth * tileMap->tileWidth),
            static_cast<float>(tileMap->mapHeight * tileMap->tileHeight));
        tileMap->render(renderer, camera.x, camera.y, camera.zoom);
        player->update();
        player->draw(camera.x, camera.y, camera.zoom);
        
        // Рисуем нашего интерактивного NPC
        if (dialogueNPC) {
            dialogueNPC->draw(camera.x, camera.y, camera.zoom);
        }
        dialogMgr.draw(renderer);
    }

    SDL_RenderPresent(renderer);
    SDL_Delay(16);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    delete tileMap;
    if (menuBackground) {
        SDL_DestroyTexture(menuBackground);
        menuBackground = nullptr;
    }
    if (menuTextTexture) SDL_DestroyTexture(menuTextTexture);
    if (menuFont) TTF_CloseFont(menuFont);
    if (disclaimerBG) {
        SDL_DestroyTexture(disclaimerBG);
        disclaimerBG = nullptr;
    }
    if (extraImage) {
        SDL_DestroyTexture(extraImage);
        extraImage = nullptr;
    }
    if (skipTextTexture) {
        SDL_DestroyTexture(skipTextTexture);
        skipTextTexture = nullptr;
    }
    delete dialogueNPC;
    if (dialogFont) {
        TTF_CloseFont(dialogFont);
        dialogFont = nullptr;
    }
    TTF_Quit();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

