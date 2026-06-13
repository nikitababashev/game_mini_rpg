#include "AudioManager.h"
#include <SDL3/SDL_log.h>

bool AudioManager::init() {
    return true;
}

void AudioManager::cleanup() {
    stopMusic();
}

bool AudioManager::playMusic(const std::string& path, int loops) {
    stopMusic();

    // 1. Загружаем WAV
    if (!SDL_LoadWAV(path.c_str(), &wavSpec, &wavBuffer, &wavLength)) {
        SDL_Log("AudioManager: ошибка загрузки %s: %s", path.c_str(), SDL_GetError());
        return false;
    }

    // 2. Открываем аудиоустройство (без указания формата)
    device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (!device) {
        SDL_Log("AudioManager: не удалось открыть устройство: %s", SDL_GetError());
        SDL_free(wavBuffer);
        wavBuffer = nullptr;
        return false;
    }

    // 3. Получаем выходной формат устройства (три аргумента)
    SDL_AudioSpec dstSpec;
    if (!SDL_GetAudioDeviceFormat(device, &dstSpec, NULL)) {
        SDL_Log("AudioManager: не удалось получить формат устройства: %s", SDL_GetError());
        SDL_CloseAudioDevice(device);
        device = 0;
        SDL_free(wavBuffer);
        wavBuffer = nullptr;
        return false;
    }

    // 4. Создаём поток с преобразованием из WAV в формат устройства
    stream = SDL_CreateAudioStream(&wavSpec, &dstSpec);
    if (!stream) {
        SDL_Log("AudioManager: ошибка создания потока: %s", SDL_GetError());
        SDL_CloseAudioDevice(device);
        device = 0;
        SDL_free(wavBuffer);
        wavBuffer = nullptr;
        return false;
    }

    // 5. Привязываем поток к устройству
    if (!SDL_BindAudioStream(device, stream)) {
        SDL_Log("AudioManager: ошибка привязки потока: %s", SDL_GetError());
        SDL_DestroyAudioStream(stream);
        stream = nullptr;
        SDL_CloseAudioDevice(device);
        device = 0;
        SDL_free(wavBuffer);
        wavBuffer = nullptr;
        return false;
    }

    // 6. Отправляем первый блок данных
    if (!SDL_PutAudioStreamData(stream, wavBuffer, wavLength)) {
        SDL_Log("AudioManager: ошибка отправки данных: %s", SDL_GetError());
        SDL_UnbindAudioStream(stream);
        SDL_DestroyAudioStream(stream);
        stream = nullptr;
        SDL_CloseAudioDevice(device);
        device = 0;
        SDL_free(wavBuffer);
        wavBuffer = nullptr;
        return false;
    }

    // 7. Запускаем воспроизведение
    SDL_ResumeAudioDevice(device);

    remainingLoops = loops;
    active = true;
    SDL_Log("AudioManager: музыка запущена (loops=%d)", loops);
    return true;
}

void AudioManager::stopMusic() {
    if (device) {
        SDL_PauseAudioDevice(device);
    }
    if (stream) {
        SDL_UnbindAudioStream(stream);
        SDL_DestroyAudioStream(stream);
        stream = nullptr;
    }
    if (device) {
        SDL_CloseAudioDevice(device);
        device = 0;
    }
    if (wavBuffer) {
        SDL_free(wavBuffer);
        wavBuffer = nullptr;
    }
    wavLength = 0;
    remainingLoops = 0;
    active = false;
}

bool AudioManager::isMusicPlaying() const {
    return active && stream && (SDL_GetAudioStreamQueued(stream) > 0 || remainingLoops != 0);
}

void AudioManager::update() {
    if (!active || !stream || !wavBuffer) return;

    int queued = SDL_GetAudioStreamQueued(stream);
    if (queued < (int)wavLength / 2) {
        if (remainingLoops > 0 || remainingLoops == -1) {
            if (!SDL_PutAudioStreamData(stream, wavBuffer, wavLength)) {
                SDL_Log("AudioManager: ошибка повторной отправки: %s", SDL_GetError());
                active = false;
                return;
            }
            if (remainingLoops > 0) remainingLoops--;
        }
    }
}
void AudioManager::setVolume(float volume) {
    currentVolume = std::max(0.0f, std::min(1.0f, volume));
    if (stream) {
        SDL_SetAudioStreamGain(stream, currentVolume);
    }
}