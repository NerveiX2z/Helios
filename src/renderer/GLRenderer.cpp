#include "GLRenderer.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

namespace {
GLuint compile(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string log(static_cast<size_t>(std::max(1, length)), '\0');
        glGetShaderInfoLog(shader, length, nullptr, &log[0]);
        std::cerr << "[GLES] Shader compile failed: " << log << '\n';
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// Column-major 4x4 matrices, matching OpenGL ES uniforms.
using Mat4 = std::array<float, 16>;
Mat4 multiply(const Mat4& a, const Mat4& b) {
    Mat4 result{};
    for (int col = 0; col < 4; ++col)
        for (int row = 0; row < 4; ++row)
            for (int k = 0; k < 4; ++k)
                result[col * 4 + row] += a[k * 4 + row] * b[col * 4 + k];
    return result;
}
Mat4 perspective(float aspect) {
    constexpr float nearPlane = 0.1f, farPlane = 100.0f;
    const float f = 1.0f / std::tan(60.0f * 3.1415926535f / 360.0f);
    return {f / aspect, 0, 0, 0,
            0, f, 0, 0,
            0, 0, (farPlane + nearPlane) / (nearPlane - farPlane), -1,
            0, 0, (2 * farPlane * nearPlane) / (nearPlane - farPlane), 0};
}
Mat4 translationScale(float x, float y, float z, float sx, float sy, float sz) {
    return {sx,0,0,0, 0,sy,0,0, 0,0,sz,0, x,y,z,1};
}
Mat4 lookAt(float ex,float ey,float ez,float tx,float ty,float tz) {
    auto normalize=[](std::array<float,3> v) {
        float n=std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
        return std::array<float,3>{v[0]/n,v[1]/n,v[2]/n};
    };
    auto cross=[](std::array<float,3> a,std::array<float,3> b) {
        return std::array<float,3>{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
    };
    auto f=normalize({tx-ex,ty-ey,tz-ez});
    auto right=normalize(cross(f,{0,1,0}));
    auto up=cross(right,f);
    Mat4 view={right[0],up[0],-f[0],0,
               right[1],up[1],-f[1],0,
               right[2],up[2],-f[2],0,
               0,0,0,1};
    return multiply(view,translationScale(-ex,-ey,-ez,1,1,1));
}
Mat4 modelView(float t) {
    const float c = std::cos(t), s = std::sin(t);
    const float cx = std::cos(-0.3f), sx = std::sin(-0.3f);
    Mat4 rotationY{c,0,-s,0, 0,1,0,0, s,0,c,0, 0,0,0,1};
    Mat4 rotationX{1,0,0,0, 0,cx,sx,0, 0,-sx,cx,0, 0,0,0,1};
    Mat4 translation{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,-5,1};
    return multiply(translation, multiply(rotationX, rotationY));
}
constexpr GLfloat vertices[] = {
    // xyz             rgb
    -1,-1,-1, 1,0,0,   1,-1,-1, 0,1,0,
     1, 1,-1, 0,0,1,  -1, 1,-1, 1,1,0,
    -1,-1, 1, 1,0,1,   1,-1, 1, 0,1,1,
     1, 1, 1, 1,1,1,  -1, 1, 1, 0.3f,0.7f,1
};
constexpr GLushort indices[] = {
    0,1,2, 0,2,3,  4,6,5, 4,7,6,
    0,4,5, 0,5,1,  3,2,6, 3,6,7,
    1,5,6, 1,6,2,  0,3,7, 0,7,4
};
}

GLRenderer::GLRenderer(int width, int height)
    : width_(std::max(1,width)), height_(std::max(1,height)) {}

GLRenderer::~GLRenderer() {
    if (context_) {
        SDL_GL_MakeCurrent(window_, context_);
        if (indexBuffer_) glDeleteBuffers(1, &indexBuffer_);
        if (vertexBuffer_) glDeleteBuffers(1, &vertexBuffer_);
        if (program_) glDeleteProgram(program_);
        SDL_GL_DeleteContext(context_);
    }
    if (window_) SDL_DestroyWindow(window_);
    if (ownsSDL_) SDL_Quit();
}

bool GLRenderer::init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "[GLES] SDL_Init: " << SDL_GetError() << '\n';
        return false;
    }
    ownsSDL_ = true;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    window_ = SDL_CreateWindow("Helios - OpenGL ES 3D",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width_, height_, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window_) {
        std::cerr << "[GLES] Window: " << SDL_GetError() << '\n';
        return false;
    }
    context_ = SDL_GL_CreateContext(window_);
    if (!context_) {
        std::cerr << "[GLES] Context: " << SDL_GetError() << '\n';
        return false;
    }
    if (SDL_GL_MakeCurrent(window_, context_) != 0) {
        std::cerr << "[GLES] MakeCurrent: " << SDL_GetError() << '\n';
        return false;
    }
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    std::cout << "[GLES] Version: " << (version ? version : "unknown") << '\n';
    if (!version || !createProgram()) return false;
    glGenBuffers(1, &vertexBuffer_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glGenBuffers(1, &indexBuffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    if (!vertexBuffer_ || !indexBuffer_ || glGetError() != GL_NO_ERROR) {
        std::cerr << "[GLES] Buffer creation failed\n";
        return false;
    }
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    SDL_GL_SetSwapInterval(1); // VSync if supported
    resize();
    return true;
}

bool GLRenderer::createProgram() {
    auto loadShader = [](const char* path) -> std::string {
        std::ifstream file(path);
        if (!file) {
            std::cerr << "[GLES] Cannot open shader: " << path << '\n';
            return {};
        }
        std::ostringstream contents;
        contents << file.rdbuf();
        return contents.str();
    };
    const std::string vertexText = loadShader("shaders/basic.vert");
    const std::string fragmentText = loadShader("shaders/basic.frag");
    if (vertexText.empty() || fragmentText.empty()) return false;
    const char* vertexSource = vertexText.c_str();
    const char* fragmentSource = fragmentText.c_str();
    GLuint vs = compile(GL_VERTEX_SHADER, vertexSource);
    if (!vs) return false;
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentSource);
    if (!fs) { glDeleteShader(vs); return false; }
    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glBindAttribLocation(program_, 0, "aPosition");
    glBindAttribLocation(program_, 1, "aColor");
    glLinkProgram(program_);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint length = 0;
        glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &length);
        std::string log(static_cast<size_t>(std::max(1, length)), '\0');
        glGetProgramInfoLog(program_, length, nullptr, &log[0]);
        std::cerr << "[GLES] Program link failed: " << log << '\n';
        return false;
    }
    positionAttr_ = glGetAttribLocation(program_, "aPosition");
    colorAttr_ = glGetAttribLocation(program_, "aColor");
    mvpUniform_ = glGetUniformLocation(program_, "uMVP");
    return positionAttr_ >= 0 && colorAttr_ >= 0 && mvpUniform_ >= 0;
}

void GLRenderer::resize() {
    if (!window_) return;
    SDL_GL_GetDrawableSize(window_, &width_, &height_);
    width_ = std::max(width_, 1);
    height_ = std::max(height_, 1);
    glViewport(0, 0, width_, height_);
}

void GLRenderer::setMap(const std::vector<std::vector<int>>& map) {
    map_ = map;
}

void GLRenderer::setStressCount(int count) {
    stressCount_ = std::clamp(count, 0, 5000);
}

void GLRenderer::render(float elapsedSeconds) {
    glClearColor(0.04f, 0.07f, 0.13f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const float aspect = static_cast<float>(width_) / height_;
    const Mat4 projection = perspective(aspect);
    const float cx = map_.empty() ? 0.0f : static_cast<float>(map_[0].size()) * 0.5f;
    const float cz = static_cast<float>(map_.size()) * 0.5f;
    const float radius = std::max(9.0f, std::max(cx,cz)*2.3f);
    const Mat4 view = lookAt(cx + std::sin(elapsedSeconds*0.12f)*radius,
                             radius*0.8f,
                             cz + std::cos(elapsedSeconds*0.12f)*radius,
                             cx, 0.0f, cz);
    const Mat4 vp = multiply(projection,view);
    glUseProgram(program_);
    
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
    glEnableVertexAttribArray(static_cast<GLuint>(positionAttr_));
    glEnableVertexAttribArray(static_cast<GLuint>(colorAttr_));
    glVertexAttribPointer(static_cast<GLuint>(positionAttr_), 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(GLfloat), reinterpret_cast<const void*>(0));
    glVertexAttribPointer(static_cast<GLuint>(colorAttr_), 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(GLfloat), reinterpret_cast<const void*>(3 * sizeof(GLfloat)));
    auto drawBox = [&](float x,float y,float z,float sx,float sy,float sz) {
        Mat4 mvp=multiply(vp,translationScale(x,y,z,sx,sy,sz));
        glUniformMatrix4fv(mvpUniform_,1,GL_FALSE,mvp.data());
        glDrawElements(GL_TRIANGLES,36,GL_UNSIGNED_SHORT,nullptr);
    };
    if (map_.empty()) {
        Mat4 mvp=multiply(projection,modelView(elapsedSeconds));
        glUniformMatrix4fv(mvpUniform_,1,GL_FALSE,mvp.data());
        glDrawElements(GL_TRIANGLES,36,GL_UNSIGNED_SHORT,nullptr);
    } else {
        // One floor slab plus solid 3D cubes for map walls.
        drawBox(cx,-0.13f,cz,cx,0.1f,cz);
        for (size_t z=0; z<map_.size(); ++z)
            for (size_t x=0; x<map_[z].size(); ++x)
                if (map_[z][x] != 0)
                    drawBox(static_cast<float>(x)+0.5f,0.9f,
                            static_cast<float>(z)+0.5f,0.5f,0.9f,0.5f);
    }
    // Benchmark geometry: deliberately separate draw calls to measure CPU
    // submission + GPU work. Deterministic grid; no random allocations.
    if (stressCount_ > 0) {
        const int cols = 25;
        const int rows = (stressCount_ + cols - 1) / cols;
        for (int i = 0; i < stressCount_; ++i) {
            const int col = i % cols;
            const int row = i / cols;
            const float x = cx + (col - 12) * 0.65f;
            const float z = cz + (row - rows * 0.5f) * 0.65f;
            const float y = 1.8f + 0.2f * std::sin(elapsedSeconds + i * 0.1f);
            drawBox(x, y, z, 0.20f, 0.20f, 0.20f);
        }
    }
    SDL_GL_SwapWindow(window_);
}
