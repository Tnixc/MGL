#ifndef RAY_TRACER_H
#define RAY_TRACER_H

#define GL_GLEXT_PROTOTYPES 1
#include <GL/glcorearb.h>
#include <SDL2/SDL.h>

extern "C"
{
#include "MGLContext.h"
#include "MGLRenderer.h"
}

class RayTracer
{
  private:
    GLMContext ctx;
    SDL_Window *window;
    int width = 800;
    int height = 600;

    GLuint compute_program;
    GLuint texture;
    GLuint vao, vbo, ebo;
    GLuint render_program;
    GLuint camera_ubo;

    float time = 0.0f;

    bool setupShaders();
    bool setupTexture();
    bool setupGeometry();
    void setupCameraUniform();
    void updateComputeShader();
    void render();

  public:
    RayTracer();
    ~RayTracer();

    bool initialize();
    void run();
    void cleanup();
};

#endif // RAY_TRACER_H
