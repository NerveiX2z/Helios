#include <SDL2/SDL.h>
#include <cstdio>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "Engine.h"
#include "GLRenderer.h"
#include "Player.h"

namespace {
std::vector<std::vector<int>> fallbackMap() {
    return {{1,1,1,1,1,1,1,1,1,1},
            {1,0,0,0,0,0,0,0,0,1},
            {1,0,1,1,0,0,1,1,0,1},
            {1,0,0,0,0,0,0,0,0,1},
            {1,1,1,1,1,1,1,1,1,1}};
}
bool validMap(const std::vector<std::vector<int>>& map) {
    if (map.size() < 3 || map[0].size() < 3) return false;
    const size_t width = map[0].size();
    for (const auto& row : map) {
        if (row.size() != width || row.front() != 1 || row.back() != 1) return false;
        for (int cell : row) if (cell != 0 && cell != 1) return false;
    }
    for (size_t x = 0; x < width; ++x)
        if (map.front()[x] != 1 || map.back()[x] != 1) return false;
    return true;
}
std::vector<std::vector<int>> loadMap(const std::string& path) {
    std::ifstream file(path);
    std::vector<std::vector<int>> map;
    std::string line;
    while (std::getline(file, line)) {
        std::vector<int> row;
        for (char c : line) if (c == '0' || c == '1') row.push_back(c - '0');
        if (!row.empty()) map.push_back(row);
    }
    if (!validMap(map)) {
        std::cerr << "[WARN] Invalid/unavailable map; using fallback.\n";
        return fallbackMap();
    }
    return map;
}
}

int main(int argc, char** argv) {
    bool legacy = false, stress = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--raycaster") legacy = true;
        else if (arg == "--test" || arg == "--stress") stress = true;
        else {
            std::cerr << "Usage: ./helios [--test | --stress | --raycaster]\n";
            return 2;
        }
    }
    if (legacy && stress) {
        std::cerr << "[ERROR] --test requires GLES mode\n";
        return 2;
    }
    std::cout << "[Helios] Renderer: " << (legacy ? "legacy raycaster" : "OpenGL ES 2.0") << '\n';
    std::unique_ptr<GLRenderer> gl;
    std::unique_ptr<Engine> cpu;
    if (legacy) {
        cpu = std::make_unique<Engine>(1280, 720);
        if (!cpu->init()) return 1;
    } else {
        gl = std::make_unique<GLRenderer>(1280, 720);
        if (!gl->init()) {
            std::cerr << "[ERROR] OpenGL ES failed. Try ./helios --raycaster\n";
            return 1;
        }
    }
    auto map = loadMap("assets/maps/map.txt");
    if (gl) gl->setMap(map);
    constexpr Uint64 refreshIntervalMs = 5000;
    Uint64 lastRefresh = SDL_GetTicks64();
    const Uint64 stressStart = SDL_GetTicks64();
    int stressStage = -1;
    const int stressCounts[] = {0, 100, 500, 1000, 5000};
    Player player(1.5, 1.5);
    bool running = true;
    Uint64 frames = 0;
    Uint64 statsStart = SDL_GetTicks64();
    Uint64 last = SDL_GetPerformanceCounter();
    const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());
    double elapsed = 0;
    while (running) {
        const Uint64 frameStart = SDL_GetPerformanceCounter();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT ||
                (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
                running = false;
            if (gl && event.type == SDL_WINDOWEVENT &&
                (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                 event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)) gl->resize();
        }
        if (!running) break;
        const Uint64 now = SDL_GetPerformanceCounter();
        double dt = static_cast<double>(now - last) / frequency;
        last = now;
        if (dt > 0.1) dt = 0.1;
        elapsed += dt;
        if (legacy) {
            player.update(map, dt);
            cpu->renderRaycaster(player.x, player.y, player.dirX, player.dirY,
                                 player.planeX, player.planeY, map);
            if (!cpu->present()) break;
        } else {
            gl->render(static_cast<float>(elapsed));
        }
        ++frames;
        const Uint64 ticks = SDL_GetTicks64();
        if (stress && gl) {
            const int stage = std::min(4, static_cast<int>((ticks - stressStart) / 10000));
            if (stage != stressStage) {
                stressStage = stage;
                gl->setStressCount(stressCounts[stage]);
                std::cout << "[STRESS] Stage " << stage
                          << " | Extra cubes: " << stressCounts[stage] << '\n';
                frames = 0;
                statsStart = ticks;
            }
        }
        if (ticks - lastRefresh >= refreshIntervalMs) {
            if (gl) {
                map = loadMap("assets/maps/map.txt");
                gl->setMap(map);
                std::cout << "[MAP] Reloaded " << map.size() << " rows x "
                          << map.front().size() << " columns\n";
            }
            lastRefresh = ticks;
        }
        if (ticks - statsStart >= refreshIntervalMs) {
            const double seconds = (ticks - statsStart) / 1000.0;
            std::cout << "[STATS] FPS: " << frames / seconds
                      << " | Frame ms: " << 1000.0 * seconds / std::max<Uint64>(1,frames)
                      << " | Extra cubes: " << (gl ? gl->stressCount() : 0)
                      << '\n';
            frames = 0;
            statsStart = ticks;
        }
        const double used = (SDL_GetPerformanceCounter() - frameStart) / frequency;
        if (used < 1.0 / 60.0) SDL_Delay(static_cast<Uint32>((1.0 / 60.0 - used) * 1000));
    }
    return 0;
}
