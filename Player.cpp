#include "Player.h"
#include<SDL3_image/SDL_image.h>

int where_see_idle = 0;


void Player::handleEvents() {

}

void Player::draw() {
	SDL_RenderTexture(renderer, texture, &src, &dest);
}

void Player::showAnimation(animation animation, int now, int delay) {
	if (delay >= animation.animationDelay) {
		lastUpdate = now;
		currentIndex = (currentIndex + 1) % animation.frames;
		src.x = currentIndex * 64;
		src.y = 128 * animation.y;
	}
}
void Player::update(){
	const bool* keys = SDL_GetKeyboardState(NULL);

	Uint64 now = SDL_GetTicks();//время инициализации и текущее
	int delay = now - lastUpdate;
	
	if (keys[SDL_SCANCODE_S] and not keys[SDL_SCANCODE_LSHIFT]) {
		dest.y += speed;
		showAnimation(animations.walk_w, now, delay);
		where_see_idle = 0;
	}
	else if (keys[SDL_SCANCODE_A] and not keys[SDL_SCANCODE_LSHIFT]) {
		dest.x -= speed;
		showAnimation(animations.walk_a, now, delay);
		where_see_idle = 1;
	}
	else if (keys[SDL_SCANCODE_D] and not keys[SDL_SCANCODE_LSHIFT]) {
		dest.x += speed;
		showAnimation(animations.walk_d, now, delay);
		where_see_idle = 2;
	}
	else if (keys[SDL_SCANCODE_W] and not keys[SDL_SCANCODE_LSHIFT]) {
		dest.y -= speed;
		showAnimation(animations.walk_s, now, delay);
		where_see_idle = 3;
	}
	else if (keys[SDL_SCANCODE_W] and keys[SDL_SCANCODE_LSHIFT]) {
		dest.y -= walk_speed;
		showAnimation(animations.run_s, now, delay);
	}
	else if (keys[SDL_SCANCODE_S] and keys[SDL_SCANCODE_LSHIFT]) {
		dest.y += walk_speed;
		showAnimation(animations.run_w, now, delay);
	}
	else if (keys[SDL_SCANCODE_A] and keys[SDL_SCANCODE_LSHIFT]) {
		dest.x -= walk_speed;
		showAnimation(animations.run_a, now, delay);
	}
	else if (keys[SDL_SCANCODE_D] and keys[SDL_SCANCODE_LSHIFT]) {
		dest.x += walk_speed;
		showAnimation(animations.run_d, now, delay);
	}
	else {
		if (where_see_idle == 0) {
			showAnimation(animations.idle_w, now, delay);
		}
		else if (where_see_idle == 1) {
			showAnimation(animations.idle_a, now, delay);
		}
		else if (where_see_idle == 2) {
			showAnimation(animations.idle_d, now, delay);
		}
		else if (where_see_idle == 3) {
			showAnimation(animations.idle_s, now, delay);
		}
	}

}
void Player::initAnimations() {
	animations.idle_w = { 8, 100, 0 };
	animations.idle_a = { 8, 100, 1 };
	animations.idle_d = { 8, 100, 2 };
	animations.idle_s = { 8, 100, 3 };

	animations.walk_w = { 8, 100, 4 };
	animations.walk_a = { 8, 100, 5 };
	animations.walk_d = { 8, 100, 6 };
	animations.walk_s = { 8, 100, 7 };

	animations.run_w = { 8, 100, 8 };
	animations.run_a = { 8, 100, 9 };
	animations.run_d = { 8, 100, 10 };
	animations.run_s = { 8, 100, 11};


	
}

Player::Player(SDL_Renderer* renderer, std::string texturePath): renderer(renderer)
{

	
	initAnimations();
	sizeSprite = 64;
	texture = IMG_LoadTexture(renderer, texturePath.c_str());
	dest = { 0, 0, 64 * 1.5, 64 * 3.5 };
	src = { 0, 0, 64, 64 * 2 };
	walk_speed = 5;
	speed = 2;

}

Player::~Player()
{
	SDL_DestroyTexture(texture);
}
