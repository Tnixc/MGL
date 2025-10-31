
#include <GL/glcorearb.h>
#include <print>
#include <random>

#define GL_GLEXT_PROTOTYPES 1

#include <SDL2/SDL.h>
#include "MGLRenderer.h"
#include <SDL2/SDL_syswm.h>

extern "C"
{
#include "MGLContext.h"
}

#define GLSL(version, shader) "#version " #version "\n" #shader

GLuint compileComputeShader(const char *shader_source)
{
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &shader_source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::print(stderr, "Compute shader compilation failed:\n{}", infoLog);
        std::print(stderr, "Shader source:\n{}", shader_source);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glDeleteShader(shader);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::print(stderr, "Program linking failed:\n{}", infoLog);
        return 0;
    }

    return program;
}

GLuint compileGLSLProgram(int count, ...)
{
    va_list args;
    va_start(args, count);

    GLuint program = glCreateProgram();

    for (int i = 0; i < count; i++)
    {
        GLenum shader_type = va_arg(args, GLenum);
        const char *shader_source = va_arg(args, const char *);

        GLuint shader = glCreateShader(shader_type);
        glShaderSource(shader, 1, &shader_source, NULL);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::print(stderr, "Shader compilation failed:\n{}", infoLog);
            std::print(stderr, "Shader source:\n{}", shader_source);
            return 0;
        }

        glAttachShader(program, shader);
        glDeleteShader(shader);
    }

    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::print(stderr, "Program linking failed:\n{}", infoLog);
        return 0;
    }

    va_end(args);
    return program;
}

class ConwayGameOfLife
{
  private:
    GLMContext ctx;
    SDL_Window *window;

    // Grid dimensions (powers of two for texture wrapping)
    static constexpr int GRID_WIDTH = 512;
    static constexpr int GRID_HEIGHT = 512;
    static constexpr int CELL_SIZE = 2; // Scale up for visibility

    // Compute shader work group size
    static constexpr int WORK_GROUP_SIZE = 16;

    int window_width = GRID_WIDTH * CELL_SIZE;
    int window_height = GRID_HEIGHT * CELL_SIZE;

    GLuint textures[2]; // front and back
    GLuint quad_vbo, quad_vao;
    GLuint gol_compute_program; // Game of Life compute shader
    GLuint display_program;     // For rendering to screen

    int current_texture = 0; // 0 for front, 1 for back
    int generation = 0;

    const char *gol_compute_shader_source = GLSL(
        430 core, layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

        layout(rgba8, binding = 0) uniform readonly image2D inputImage;
        layout(rgba8, binding = 1) uniform writeonly image2D outputImage;

        void main() {
            ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
            ivec2 size = imageSize(inputImage);

            // Bounds check - don't process out of bounds pixels
            if (texelCoord.x >= size.x || texelCoord.y >= size.y)
            {
                return;
            }

            int sum = 0;
            for (int dy = -1; dy <= 1; dy++)
            {
                for (int dx = -1; dx <= 1; dx++)
                {
                    if (dx == 0 && dy == 0)
                        continue;

                    // Proper toroidal wrapping
                    ivec2 neighbor =
                        ivec2((texelCoord.x + dx + size.x) % size.x, (texelCoord.y + dy + size.y) % size.y);

                    float cell = imageLoad(inputImage, neighbor).r;
                    sum += int(cell > 0.5); // Use threshold instead of direct cast
                }
            }

            float current = imageLoad(inputImage, texelCoord).r;
            bool alive = current > 0.5;
            bool nextAlive = false;

            if (sum == 3)
            {
                nextAlive = true; // Birth or survival
            }
            else if (sum == 2 && alive)
            {
                nextAlive = true; // Survival only
            }

            float next = nextAlive ? 1.0 : 0.0;
            imageStore(outputImage, texelCoord, vec4(next, next, next, 1.0));
        });

    const char *vertex_shader_source = GLSL(
        460 core, layout(location = 0) in vec2 position; layout(location = 0) out vec2 texCoord;

        void main() {
            gl_Position = vec4(position, 0.0, 1.0);
            texCoord = position * 0.5 + 0.5;
        });

    const char *display_fragment_shader_source = GLSL(
        460 core, layout(location = 0) uniform sampler2D state; layout(location = 0) in vec2 texCoord;
        layout(location = 0) out vec4 fragColor;

        void main() { fragColor = texture(state, texCoord); });

  public:
    ConwayGameOfLife() : ctx(nullptr), window(nullptr)
    {
    }

    ~ConwayGameOfLife()
    {
        cleanup();
    }

    bool initialize()
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            std::print(stderr, "SDL initialization failed: {}\n", SDL_GetError());
            return false;
        }

        window =
            SDL_CreateWindow("Conway's Game of Life (Compute Shader)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             window_width, window_height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
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
        SDL_GL_SetSwapInterval(0);

        int w, h, wscaled, hscaled;
        SDL_GetWindowSize(window, &w, &h);
        SDL_GL_GetDrawableSize(window, &wscaled, &hscaled);

        glViewport(0, 0, wscaled, hscaled);

        return true;
    }

    bool setupShaders()
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        MGLswapBuffers(ctx);

        gol_compute_program = compileComputeShader(gol_compute_shader_source);
        if (!gol_compute_program)
        {
            std::print(stderr, "Failed to compile GOL compute shader\n");
            return false;
        }

        display_program = compileGLSLProgram(2, GL_VERTEX_SHADER, vertex_shader_source, GL_FRAGMENT_SHADER,
                                             display_fragment_shader_source);
        if (!display_program)
        {
            std::print(stderr, "Failed to compile display shaders\n");
            return false;
        }

        return true;
    }

    bool setupTextures()
    {
        glGenTextures(2, textures);
        for (int i = 0; i < 2; i++)
        {
            glBindTexture(GL_TEXTURE_2D, textures[i]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, GRID_WIDTH, GRID_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }

        // Initialize front texture with random pattern
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(0, 1);

        std::vector<uint8_t> initial_state(GRID_WIDTH * GRID_HEIGHT * 4);
        for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; i++)
        {
            uint8_t val = dis(gen) * 255;
            initial_state[i * 4 + 0] = val;
            initial_state[i * 4 + 1] = val;
            initial_state[i * 4 + 2] = val;
            initial_state[i * 4 + 3] = 255;
        }

        glBindTexture(GL_TEXTURE_2D, textures[0]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GRID_WIDTH, GRID_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE,
                        initial_state.data());

        return true;
    }

    bool setupGeometry()
    {
        float quad_vertices[] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f};

        glGenVertexArrays(1, &quad_vao);
        glGenBuffers(1, &quad_vbo);

        glBindVertexArray(quad_vao);
        glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);

        return true;
    }

    void swapTextures()
    {
        current_texture = 1 - current_texture;
    }

    void updateGeneration()
    {
        int front = current_texture;
        int back = 1 - current_texture;

        // Bind textures as images for compute shader
        glBindImageTexture(0, textures[front], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
        glBindImageTexture(1, textures[back], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

        // Use compute shader
        glUseProgram(gol_compute_program);

        // Dispatch compute shader
        int num_groups_x = (GRID_WIDTH + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;
        int num_groups_y = (GRID_HEIGHT + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;
        glDispatchCompute(num_groups_x, num_groups_y, 1);

        // Memory barrier to ensure writes are complete
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // Swap
        swapTextures();

        generation++;
    }

    void render()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        int w, h;
        SDL_GL_GetDrawableSize(window, &w, &h);
        glViewport(0, 0, w, h);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Bind current texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[current_texture]);

        // Use display program
        glUseProgram(display_program);
        glUniform1i(0, 0);

        // Draw quad
        glBindVertexArray(quad_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        MGLswapBuffers(ctx);
    }

    void randomizeGrid()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(0, 1);

        std::vector<uint8_t> new_state(GRID_WIDTH * GRID_HEIGHT * 4);
        for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; i++)
        {
            uint8_t val = dis(gen) * 255;
            new_state[i * 4 + 0] = val;
            new_state[i * 4 + 1] = val;
            new_state[i * 4 + 2] = val;
            new_state[i * 4 + 3] = 255;
        }

        glBindTexture(GL_TEXTURE_2D, textures[current_texture]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GRID_WIDTH, GRID_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, new_state.data());
        generation = 0;
    }

    void run()
    {
        if (!setupShaders())
        {
            std::print(stderr, "Failed to setup shaders\n");
            return;
        }

        if (!setupTextures())
        {
            std::print(stderr, "Failed to setup textures\n");
            return;
        }

        if (!setupGeometry())
        {
            std::print(stderr, "Failed to setup geometry\n");
            return;
        }

        bool running = true;
        bool paused = false;
        SDL_Event event;
        Uint32 last_update = SDL_GetTicks();
        const Uint32 update_interval = 160; // ~60 updates per second

        std::print("Conway's Game of Life (Compute Shader)\n");
        std::print("Grid: {}x{} cells ({}x{} pixels)\n", GRID_WIDTH, GRID_HEIGHT, window_width, window_height);
        std::print("Cell size: {}x{} pixels\n", CELL_SIZE, CELL_SIZE);
        std::print("\nControls:\n");
        std::print("  SPACE - Pause/Resume\n");
        std::print("  R     - Randomize grid\n");
        std::print("  ESC   - Exit\n\n");

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
                    switch (event.key.keysym.sym)
                    {
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                    case SDLK_SPACE:
                        paused = !paused;
                        std::print("{}\n", paused ? "Paused" : "Resumed");
                        break;
                    case SDLK_r:
                        randomizeGrid();
                        std::print("Grid randomized\n");
                        break;
                    }
                }
            }

            Uint32 current_time = SDL_GetTicks();

            // Update simulation
            if (!paused && current_time - last_update >= update_interval)
            {
                updateGeneration();
                last_update = current_time;

                if (generation % 10 == 0)
                {
                    std::print("Generation: {}\n", generation);
                }
            }

            // Always render
            render();

            SDL_Delay(1);
        }
    }

    void cleanup()
    {
        if (window)
        {
            SDL_DestroyWindow(window);
            window = nullptr;
        }

        SDL_Quit();
    }
};

int main(int argc, char **argv)
{
    ConwayGameOfLife game;

    if (!game.initialize())
    {
        std::print(stderr, "Failed to initialize Conway's Game of Life\n");
        return 1;
    }

    game.run();

    return 0;
}
