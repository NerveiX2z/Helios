#ifndef ENGINE_H
#define ENGINE_H

#include <SDL2/SDL.h>
#include <vector>

class Engine {
public:
    Engine(int dispWidth, int dispHeight);
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    bool init();
    void clearScreen();
    void renderRaycaster(double posX, double posY, double dirX, double dirY,
                         double planeX, double planeY,
                         const std::vector<std::vector<int>>& map);
    bool present();

private:
    int dispWidth;
    int dispHeight;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* screenTexture;
    bool sdlInitialized;
    std::vector<Uint32> colorBuffer;
};

#endif
