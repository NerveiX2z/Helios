#include "../include/Player.h"

#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>

namespace {
bool isWall(const std::vector<std::vector<int>>& map, int x, int y) {
    if (y < 0 || y >= static_cast<int>(map.size())) {
        return true;
    }
    if (x < 0 || x >= static_cast<int>(map[y].size())) {
        return true;
    }
    return map[y][x] != 0;
}

void rotateVector(double& x, double& y, double angle) {
    const double oldX = x;
    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);
    x = x * cosine - y * sine;
    y = oldX * sine + y * cosine;
}
}

Player::Player(double startX, double startY)
    : x(startX),
      y(startY),
      dirX(-1.0),
      dirY(0.0),
      planeX(0.0),
      planeY(0.66),
      speed(3.0),       // map units per second; equivalent to 0.05/frame at 60 FPS
      rotSpeed(1.8) {}  // radians per second; equivalent to 0.03/frame at 60 FPS

void Player::update(const std::vector<std::vector<int>>& map, double deltaTime) {
    if (map.empty()) return;

    // Avoid large teleports after the app is paused or backgrounded.
    deltaTime = std::clamp(deltaTime, 0.0, 0.1);
    if (deltaTime <= 0.0) return;

    const Uint8* state = SDL_GetKeyboardState(nullptr);
    const bool forward = state[SDL_SCANCODE_W] || state[SDL_SCANCODE_UP];
    const bool backward = state[SDL_SCANCODE_S] || state[SDL_SCANCODE_DOWN];
    const double moveAmount = speed * deltaTime;

    auto move = [&](double amount) {
        const double nextX = x + dirX * amount;
        const double nextY = y + dirY * amount;

        // Resolve each axis separately to allow sliding along walls.
        if (!isWall(map, static_cast<int>(nextX), static_cast<int>(y))) {
            x = nextX;
        }
        if (!isWall(map, static_cast<int>(x), static_cast<int>(nextY))) {
            y = nextY;
        }
    };

    if (forward) move(moveAmount);
    if (backward) move(-moveAmount);

    if (state[SDL_SCANCODE_D] || state[SDL_SCANCODE_RIGHT]) {
        const double angle = -rotSpeed * deltaTime;
        rotateVector(dirX, dirY, angle);
        rotateVector(planeX, planeY, angle);
    }
    if (state[SDL_SCANCODE_A] || state[SDL_SCANCODE_LEFT]) {
        const double angle = rotSpeed * deltaTime;
        rotateVector(dirX, dirY, angle);
        rotateVector(planeX, planeY, angle);
    }
}
