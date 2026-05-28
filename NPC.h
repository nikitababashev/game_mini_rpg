#pragma once
#include<SDL3/SDL.h>
#include<string>

typedef struct animationnpc {//новый тип данных
	int frames;
	int animationDelay;
	int y;
};

typedef struct animationDatanpc {//новый тип данных
	animationnpc idle;
};

class NPC
{
public:
	NPC(SDL_Renderer* renderer, std::string texturePath);
	void draw();
	void update();
	void handleEvents();
	~NPC();

private:
	void initAnimations();
	void showAnimationnpc(animationnpc animation, int now, int delay);
	SDL_Renderer* renderer;
	SDL_FRect dest;
	SDL_FRect src;
	SDL_Texture* texture;
	int speed;
	int walk_speed;
	animationnpc idle;
	animationDatanpc animations;
	int currentIndex;
	int lastUpdate;
	float sizeSprite;
	int isIdle;
	int isWalk;
	int where_see_idle;
	SDL_FlipMode flip;
};