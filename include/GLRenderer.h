#pragma once
#include <SDL2/SDL.h>
#include <GLES2/gl2.h>
#include <vector>

class GLRenderer {
public:
    GLRenderer(int width, int height);
    ~GLRenderer();
    GLRenderer(const GLRenderer&) = delete;
    GLRenderer& operator=(const GLRenderer&) = delete;
    bool init();
    void render(float elapsedSeconds);
    void setMap(const std::vector<std::vector<int>>& map);
    void setStressCount(int count);
    int stressCount() const { return stressCount_; }
    void resize();
private:
    bool createProgram();
    std::vector<std::vector<int>> map_;
    int width_, height_;
    int stressCount_ = 0;
    SDL_Window* window_ = nullptr;
    SDL_GLContext context_ = nullptr;
    bool ownsSDL_ = false;
    GLuint program_ = 0, vertexBuffer_ = 0, indexBuffer_ = 0;
    GLint positionAttr_ = -1, colorAttr_ = -1, mvpUniform_ = -1;
};
