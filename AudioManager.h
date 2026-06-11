#pragma once
#include <SDL3/SDL.h>
#include <string>

class AudioManager {
public:
    bool init();
    void cleanup();

    bool playMusic(const std::string& path, int loops = -1);
    void stopMusic();
    void update();               // нужно вызывать каждый кадр
    bool isMusicPlaying() const;

private:
    SDL_AudioDeviceID device = 0;
    SDL_AudioStream* stream = nullptr;
    Uint8* wavBuffer = nullptr;
    Uint32 wavLength = 0;
    SDL_AudioSpec wavSpec;
    int remainingLoops = 0;      // -1 = бесконечно
    bool active = false;
};