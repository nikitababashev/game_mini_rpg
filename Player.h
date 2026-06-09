#pragma once
#include<SDL3/SDL.h>
#include<string>
struct animation {//новый тип данных
	int frames;
	int animationDelay;
	int y;
};

struct animationData {//новый тип данных
	animation walk_w;
	animation walk_a;
	animation walk_d;
	animation walk_s;

	animation idle_w;
	animation idle_a;
	animation idle_d;
	animation idle_s;

	animation run_w;
	animation run_a;
	animation run_d;
	animation run_s;
};

class Player
{
public:
	Player(SDL_Renderer* renderer, std::string texturePath);
	void draw(float camX, float camY, float zoom);
	void update();
	void handleEvents();
	float worldX = 0.0f;
	float worldY = 0.0f;
	~Player();

private:
	void initAnimations();
	float posX; 
	float posY; // начальная позиция игрока
	
	void showAnimation(animation animation, int now, int delay);
	SDL_Renderer* renderer;
	SDL_FRect dest;
	SDL_FRect src;
	SDL_Texture* texture;
	int speed;
	int walk_speed;
	animation idle;
	animationData animations;
	int currentIndex;
	int lastUpdate;
	float sizeSprite;
	int isIdle;
	int isWalk;
	int where_see_idle;
	SDL_FlipMode flip;
};