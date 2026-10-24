#include "../include/Engine.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

Engine::Engine(int w, int h)
    : dispWidth(w > 0 ? w : 1),
      dispHeight(h > 0 ? h : 1),
      window(nullptr),
      renderer(nullptr),
      screenTexture(nullptr),
      sdlInitialized(false),
      colorBuffer(static_cast<std::size_t>(dispWidth) *
                  static_cast<std::size_t>(dispHeight), 0) {}

Engine::~Engine() {
    if (screenTexture) SDL_DestroyTexture(screenTexture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    if (sdlInitialized) SDL_Quit();
}

bool Engine::init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "[ERROR] SDL_Init failed: " << SDL_GetError() << '\n';
        return false;
    }
    sdlInitialized = true;

    window = SDL_CreateWindow(
        "HELIOS MODULAR ENGINE",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        dispWidth, dispHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        std::cerr << "[ERROR] SDL_CreateWindow failed: "
                  << SDL_GetError() << '\n';
        return false;
    }

    renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        std::cerr << "[ERROR] Accelerated renderer failed: "
                  << SDL_GetError() << '\n';
        // Software fallback improves compatibility with some Termux setups.
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer) {
        std::cerr << "[ERROR] SDL_CreateRenderer failed: "
                  << SDL_GetError() << '\n';
        return false;
    }

    if (SDL_RenderSetLogicalSize(renderer, dispWidth, dispHeight) < 0) {
        std::cerr << "[ERROR] SDL_RenderSetLogicalSize failed: "
                  << SDL_GetError() << '\n';
        return false;
    }

    screenTexture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        dispWidth, dispHeight
    );
    if (!screenTexture) {
        std::cerr << "[ERROR] SDL_CreateTexture failed: "
                  << SDL_GetError() << '\n';
        return false;
    }

    return true;
}

void Engine::clearScreen() {
    const Uint32 skyColor = 0xFF0F1423;
    const Uint32 groundColor = 0xFF225522;
    const int halfHeight = dispHeight / 2;

    for (int y = 0; y < halfHeight; ++y) {
        std::fill(colorBuffer.begin() + static_cast<std::size_t>(y) * dispWidth,
                  colorBuffer.begin() + static_cast<std::size_t>(y + 1) * dispWidth,
                  skyColor);
    }
    for (int y = halfHeight; y < dispHeight; ++y) {
        std::fill(colorBuffer.begin() + static_cast<std::size_t>(y) * dispWidth,
                  colorBuffer.begin() + static_cast<std::size_t>(y + 1) * dispWidth,
                  groundColor);
    }
}

void Engine::renderRaycaster(
    double posX, double posY, double dirX, double dirY,
    double planeX, double planeY,
    const std::vector<std::vector<int>>& map
) {
    clearScreen();

    if (map.empty() || map.front().empty()) {
        std::cerr << "[WARN] Raycaster received an empty map.\n";
        return;
    }

    const int mapHeight = static_cast<int>(map.size());
    const int mapWidth = static_cast<int>(map.front().size());
    for (const auto& row : map) {
        if (static_cast<int>(row.size()) != mapWidth) {
            std::cerr << "[WARN] Raycaster received a non-rectangular map.\n";
            return;
        }
    }

    for (int x = 0; x < dispWidth; ++x) {
        const double cameraX = 2.0 * x / static_cast<double>(dispWidth) - 1.0;
        const double rayDirX = dirX + planeX * cameraX;
        const double rayDirY = dirY + planeY * cameraX;

        int mapX = static_cast<int>(posX);
        int mapY = static_cast<int>(posY);

        const double deltaDistX = (rayDirX == 0.0)
            ? std::numeric_limits<double>::max() / 4.0
            : std::abs(1.0 / rayDirX);
        const double deltaDistY = (rayDirY == 0.0)
            ? std::numeric_limits<double>::max() / 4.0
            : std::abs(1.0 / rayDirY);

        double sideDistX;
        double sideDistY;
        double perpWallDist = 10.0;
        int stepX;
        int stepY;
        int side = 0;
        bool hitWall = false;
        bool hitBoundary = false;

        if (rayDirX < 0.0) {
            stepX = -1;
            sideDistX = (posX - mapX) * deltaDistX;
        } else {
            stepX = 1;
            sideDistX = (mapX + 1.0 - posX) * deltaDistX;
        }

        if (rayDirY < 0.0) {
            stepY = -1;
            sideDistY = (posY - mapY) * deltaDistY;
        } else {
            stepY = 1;
            sideDistY = (mapY + 1.0 - posY) * deltaDistY;
        }

        // A ray cannot cross more than width + height cells before leaving
        // the map. Keep a guard anyway so malformed maps cannot hang a frame.
        const int maxSteps = mapWidth + mapHeight + 1;
        for (int steps = 0; steps < maxSteps && !hitWall; ++steps) {
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }

            if (mapX < 0 || mapX >= mapWidth ||
                mapY < 0 || mapY >= mapHeight) {
                hitBoundary = true;
                hitWall = true;
            } else if (map[mapY][mapX] > 0) {
                hitWall = true;
            }
        }

        if (hitWall && !hitBoundary) {
            perpWallDist = (side == 0)
                ? sideDistX - deltaDistX
                : sideDistY - deltaDistY;
        }

        if (!std::isfinite(perpWallDist) || perpWallDist < 0.001) {
            perpWallDist = 0.001;
        }

        const int lineHeight = static_cast<int>(
            dispHeight / (perpWallDist + 0.0001)
        );
        int drawStart = -lineHeight / 2 + dispHeight / 2;
        int drawEnd = lineHeight / 2 + dispHeight / 2;
        drawStart = std::max(0, drawStart);
        drawEnd = std::min(dispHeight - 1, drawEnd);

        Uint8 shade = static_cast<Uint8>(
            255.0 / (perpWallDist * 0.5 + 1.0)
        );
        if (side == 1) shade /= 2;

        const Uint32 wallColor = 0xFF000000u |
            (static_cast<Uint32>(shade) << 16) |
            (static_cast<Uint32>(shade) << 8) |
            static_cast<Uint32>(std::min(255, static_cast<int>(shade) + 25));

        for (int y = drawStart; y <= drawEnd; ++y) {
            colorBuffer[static_cast<std::size_t>(y) * dispWidth + x] = wallColor;
        }
    }
}

bool Engine::present() {
    if (!renderer || !screenTexture) {
        std::cerr << "[ERROR] Cannot present: renderer is not initialized.\n";
        return false;
    }

    if (SDL_UpdateTexture(screenTexture, nullptr, colorBuffer.data(),
                          dispWidth * static_cast<int>(sizeof(Uint32))) < 0) {
        std::cerr << "[ERROR] SDL_UpdateTexture failed: "
                  << SDL_GetError() << '\n';
        return false;
    }
    if (SDL_RenderClear(renderer) < 0) {
        std::cerr << "[ERROR] SDL_RenderClear failed: "
                  << SDL_GetError() << '\n';
        return false;
    }
    if (SDL_RenderCopy(renderer, screenTexture, nullptr, nullptr) < 0) {
        std::cerr << "[ERROR] SDL_RenderCopy failed: "
                  << SDL_GetError() << '\n';
        return false;
    }

    SDL_RenderPresent(renderer);
    return true;
}
