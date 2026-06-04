#include "Camera.h"
#include <algorithm>  // для std::clamp (C++17) или самодельные ограничения

void Camera::update(float targetX, float targetY, float worldWidth, float worldHeight,
    float screenWidth, float screenHeight) {
    // Желаемая позиция, чтобы цель оказалась в центре экрана
    float desiredX = targetX - (screenWidth / 2.0f) / zoom;
    float desiredY = targetY - (screenHeight / 2.0f) / zoom;

    // Размер видимой области
    float viewW = screenWidth / zoom;
    float viewH = screenHeight / zoom;

    // Ограничиваем, чтобы камера не выходила за края мира
    if (desiredX < 0.0f) desiredX = 0.0f;
    if (desiredY < 0.0f) desiredY = 0.0f;
    if (desiredX + viewW > worldWidth) desiredX = worldWidth - viewW;
    if (desiredY + viewH > worldHeight) desiredY = worldHeight - viewH;

    // Если мир меньше экрана – центрируем
    if (viewW >= worldWidth) desiredX = (worldWidth - viewW) / 2.0f;
    if (viewH >= worldHeight) desiredY = (worldHeight - viewH) / 2.0f;

    x = desiredX;
    y = desiredY;
}