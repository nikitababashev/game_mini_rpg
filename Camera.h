#pragma once

class Camera {
public:
    float x = 0.0f;
    float y = 0.0f;
    float zoom = 1.75f;

    // Обновляет камеру, центрируя её на targetX, targetY и ограничивая границами мира
    void update(float targetX, float targetY, float worldWidth, float worldHeight,
        float screenWidth = 1920.0f, float screenHeight = 1080.0f);
};