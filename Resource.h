#pragma once
#include<SDL3/SDL.h>

class Resource
{
public:
	Resource(SDL_Renderer* ren, const char* pathTexture);
	void draw();
	void update();
	~Resource();

private:
	SDL_Texture* texture;
	SDL_FRect dest;
	SDL_Renderer* renderer;
	SDL_FRect src;
};

