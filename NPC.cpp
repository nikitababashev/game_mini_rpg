#include "NPC.h"
#include<SDL3_image/SDL_image.h>

void NPC::handleEvents() {
		
}

void NPC::draw() {
	SDL_RenderTexture(renderer, texture, &src, &dest);
}

void NPC::showAnimationnpc(animationnpc animation, int now, int delay) {
	if (delay >= animation.animationDelay) {
		lastUpdate = now;
		currentIndex = (currentIndex + 1) % animation.frames;
		src.x = currentIndex * 46;
		src.y = 55 * animation.y;
	}
}
void NPC::update() {

	Uint64 now = SDL_GetTicks();//время инициализации и текущее
	int delay = now - lastUpdate;

	showAnimationnpc(animations.idle, now, delay);

}
void NPC::initAnimations() {
	animations.idle = { 10, 100, 0 };

}

NPC::NPC(SDL_Renderer* renderer, std::string texturePath) : renderer(renderer)
{


	initAnimations();
	sizeSprite = 55;
	texture = IMG_LoadTexture(renderer, texturePath.c_str());
	dest = { 0, 0, 46*2, 55*3};
	src = { 0, 0, 46, 55};

}

NPC::~NPC()
{
	SDL_DestroyTexture(texture);
}
