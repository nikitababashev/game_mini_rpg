#include "Resource.h"
#include<SDL3_image/SDL_image.h>

void Resource::update()
{

}
void Resource::draw()
{
	SDL_RenderTexture(renderer, texture, &src, &dest);
}

Resource::Resource(SDL_Renderer* ren, const char* pathTexture)
{
	renderer = ren;
	texture = IMG_LoadTexture(renderer, pathTexture);
	dest = { 400, 500, 0, 0 };
	src = {0, 0, 100, 100};
	SDL_GetTextureSize(texture, &dest.w, &dest.h);
}
Resource::~Resource() 
{

}
