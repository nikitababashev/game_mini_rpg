#define SDL_MAIN_USE_CALLBACKS 1
#include<SDL3/SDL.h>
#include<SDL3/SDL_main.h>
#include<iostream>
#include<stdio.h>
#include<SDL3_image/SDL_image.h>
#include "Player.h"




static SDL_Window* window;
static SDL_Renderer* renderer;
static SDL_Texture* player1;
Player* player = nullptr;
//
//64X128_Idle_Free.png
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer("oaoaoudaoda che ya nesu", 1200, 800, 0, &window, &renderer);
	player = new Player(renderer, "assets/player/split.png");
	
	

	if (!player1) {
		SDL_Log("Ошибка загрузки картинки: %s", SDL_GetError());
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	switch (event->type)
	{
	case SDL_EVENT_QUIT:
		return SDL_APP_SUCCESS;
		break;
	default:
		break;
	}
	player->handleEvents();//вызов метода при нажатии
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	SDL_SetRenderDrawColor(renderer, 77, 239, 239, 255);
	SDL_RenderClear(renderer);

	player->update();
	player->draw();
	// 3. Просто рисуем картинку. Больше тут ничего удалять нельзя!
	

	SDL_RenderPresent(renderer);
	SDL_Delay(16);

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	// 4. Правильное место для удаления текстуры — при закрытии всей игры
	if (player1) {
		SDL_DestroyTexture(player1);
	}
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
