#include "ray_tracer.h"
#include "shader_utils.h"
#include "shaders.h"
#include <print>
#include <SDL2/SDL_syswm.h>

RayTracer::RayTracer() : ctx(nullptr), window(nullptr)
{
}

RayTracer::~RayTracer()
{
    cleanup();
}

bool RayTracer::initialize()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::print(stderr, "SDL initialization failed: {}\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("Simple Ray Tracer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
                              SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window)
    {
        std::print(stderr, "Window creation failed: {}\n", SDL_GetError());
        return false;
    }

    ctx = createGLMContext(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, GL_DEPTH_COMPONENT, GL_FLOAT, 0, 0);
    if (!ctx)
    {
        std::print(stderr, "MGL context creation failed\n");
        return false;
    }

    MGLsetCurrentContext(ctx);

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (!SDL_GetWindowWMInfo(window, &info))
    {
        std::print(stderr, "Couldn't GetWindowWMInfo: {}\n", SDL_GetError());
        return false;
    }

    if (info.subsystem != SDL_SYSWM_COCOA)
    {
        std::print(stderr, "Expected Cocoa window system\n");
        return false;
    }

    void *renderer = CppCreateMGLRendererFromContextAndBindToWindow(ctx, info.info.cocoa.window);
    if (!renderer)
    {
        std::print(stderr, "Failed to create MGL renderer\n");
        return false;
    }

    SDL_SetWindowData(window, "MGLRenderer", ctx);
    SDL_GL_SetSwapInterval(1);

    int w, h, wscaled, hscaled;
    SDL_GetWindowSize(window, &w, &h);
    SDL_GL_GetDrawableSize(window, &wscaled, &hscaled);

    glViewport(0, 0, wscaled, hscaled);

    return true;
}

bool RayTracer::setupShaders()
{
    compute_program = compileGLSLProgram(1, GL_COMPUTE_SHADER, compute_shader_source);
    if (!compute_program)
    {
        std::print(stderr, "Failed to compile compute shader\n");
        return false;
    }

    render_program =
        compileGLSLProgram(2, GL_VERTEX_SHADER, vertex_shader_source, GL_FRAGMENT_SHADER, fragment_shader_source);
    if (!render_program)
    {
        std::print(stderr, "Failed to compile render shaders\n");
        return false;
    }

    return true;
}

bool RayTracer::setupTexture()
{
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureStorage2D(texture, 1, GL_RGBA32F, width, height);
    return true;
}

bool RayTracer::setupGeometry()
{
    float vertices[] = {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 1.0f, 0.0f,
                        1.0f,  1.0f,  1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 1.0f};

    unsigned int indices[] = {0, 1, 2, 2, 3, 0};

    glCreateVertexArrays(1, &vao);
    glCreateBuffers(1, &vbo);
    glCreateBuffers(1, &ebo);

    glNamedBufferStorage(vbo, sizeof(vertices), vertices, 0);
    glNamedBufferStorage(ebo, sizeof(indices), indices, 0);

    glVertexArrayVertexBuffer(vao, 0, vbo, 0, 4 * sizeof(float));
    glVertexArrayElementBuffer(vao, ebo);

    glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 0, 0);
    glEnableVertexArrayAttrib(vao, 0);

    glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
    glVertexArrayAttribBinding(vao, 1, 0);
    glEnableVertexArrayAttrib(vao, 1);

    return true;
}

void RayTracer::setupCameraUniform()
{
    glGenBuffers(1, &camera_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, camera_ubo);

    float data[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // camera_pos + time
    glBufferData(GL_UNIFORM_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBufferBase(GL_UNIFORM_BUFFER, 1, camera_ubo);
}

void RayTracer::updateComputeShader()
{
    // Update camera uniform
    float data[4] = {0.0f, 0.0f, 0.0f, time};
    glBindBuffer(GL_UNIFORM_BUFFER, camera_ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(data), data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Bind output texture
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    // Run compute shader
    glUseProgram(compute_program);
    glDispatchCompute((width + 15) / 16, (height + 15) / 16, 1);

    // Wait for compute to finish
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void RayTracer::render()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(render_program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    MGLswapBuffers(ctx);
}

void RayTracer::run()
{
    if (!setupShaders())
    {
        std::print(stderr, "Failed to setup shaders\n");
        return;
    }

    if (!setupTexture())
    {
        std::print(stderr, "Failed to setup texture\n");
        return;
    }

    if (!setupGeometry())
    {
        std::print(stderr, "Failed to setup geometry\n");
        return;
    }

    setupCameraUniform();

    bool running = true;
    SDL_Event event;
    Uint32 last_time = SDL_GetTicks();

    std::print("Simple Ray Tracer\n");
    std::print("=================\n");
    std::print("Watch the bouncing sphere!\n");
    std::print("Press ESC to exit\n");

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    running = false;
                }
            }
        }

        Uint32 current_time = SDL_GetTicks();
        float delta_time = (current_time - last_time) / 1000.0f;
        last_time = current_time;
        time += delta_time;

        updateComputeShader();
        render();
    }
}

void RayTracer::cleanup()
{
    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    SDL_Quit();
}
