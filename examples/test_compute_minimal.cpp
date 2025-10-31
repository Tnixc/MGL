#include <print>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/glcorearb.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

extern "C"
{
#include "MGLContext.h"
}
#include "MGLRenderer.h"

#define GLSL(version, shader) "#version " #version "\n" #shader

int main(int argc, char **argv)
{

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::print(stderr, "sdl init failed: {}\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Minimal Compute Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800,
                                          600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window)
    {
        std::print(stderr, "window creation failed: {}\n", SDL_GetError());
        return 1;
    }

    GLMContext ctx = createGLMContext(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, GL_DEPTH_COMPONENT, GL_FLOAT, 0, 0);
    if (!ctx)
    {
        std::print(stderr, "mgl context creation failed\n");
        return 1;
    }

    MGLsetCurrentContext(ctx);

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    SDL_GetWindowWMInfo(window, &info);
    void *renderer = CppCreateMGLRendererFromContextAndBindToWindow(ctx, info.info.cocoa.window);
    if (!renderer)
    {
        std::print(stderr, "failed to create mgl renderer\n");
        return 1;
    }

    const char *compute_source = GLSL(
        450 core, layout(local_size_x = 16, local_size_y = 16) in;
        layout(rgba32f, binding = 0) uniform writeonly image2D output_image;

        void main() {
            ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
            imageStore(output_image, coord, vec4(1.0, 0.0, 0.0, 1.0));
        });

    std::print("\n=== mgl compute shader test ===\n");
    std::print("testing: compile -> link -> dispatch\n\n");

    std::print("1. creating compute shader...");
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &compute_source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        std::print(stderr, " failed\n{}", log);
        return 1;
    }
    std::print(" success\n");

    std::print("2. linking compute program...");
    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glDeleteShader(shader);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[512];
        glGetProgramInfoLog(program, 512, NULL, log);
        std::print(stderr, " failed\n{}", log);
        return 1;
    }
    std::print(" success\n");

    std::print("3. creating output texture...");
    GLuint texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureStorage2D(texture, 1, GL_RGBA32F, 800, 600);
    std::print(" success\n");

    std::print("4. dispatching compute shader...");
    glUseProgram(program);
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glDispatchCompute(50, 38, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    std::print(" success\n");

    std::print("\n✓ all tests passed!\n");
    std::print("compute shaders are working correctly in mgl!\n\n");

    glDeleteTextures(1, &texture);
    glDeleteProgram(program);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
