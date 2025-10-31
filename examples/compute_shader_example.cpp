#include <print>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/glcorearb.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

extern "C"
{
#include "MGLContext.h"
#include "MGLRenderer.h"
}

#include <glslang/Include/glslang_c_interface.h>

#define GLSL(version, shader) "#version " #version "\n" #shader

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

class ComputeShaderExample
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

    float time = 0.0f;

    const char *compute_shader_source = GLSL(
        450 core, layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

        layout(binding = 0, rgba32f) writeonly uniform image2D output_image;

        layout(binding = 1) uniform TimeBlock { float uTime; };

        void main() {
            ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
            ivec2 size = imageSize(output_image);

            if (coord.x >= size.x || coord.y >= size.y)
                return;

            vec2 uv = (vec2(coord) / vec2(size)) * 2.0 - 1.0;
            uv.x *= float(size.x) / float(size.y);

            float dist = length(uv);
            float angle = atan(uv.y, uv.x);

            float wave1 = sin(dist * 10.0 - uTime * 5.0) * 0.5 + 0.5;
            float wave2 = sin(dist * 15.0 + angle + uTime * 3.0) * 0.3 + 0.3;

            float angular_wave = sin(angle * 6.0 + uTime * 2.0) * 0.2 + 0.2;

            float intensity = wave1 + wave2 + angular_wave;

            vec3 color =
                vec3(intensity * 0.5 + dist * 0.3, intensity * 0.8, intensity * 0.3 + 0.2 + (1.0 - dist) * 0.2);

            float center_glow = exp(-dist * 4.0);
            color += vec3(center_glow * 0.4);

            imageStore(output_image, coord, vec4(color, 1.0));
        });

    const char *vertex_shader_source = GLSL(
        450 core, layout(location = 0) in vec2 position; layout(location = 1) in vec2 texcoord;

        layout(location = 0) out vec2 v_texcoord;

        void main() {
            gl_Position = vec4(position, 0.0, 1.0);
            v_texcoord = texcoord;
        });

    const char *fragment_shader_source = GLSL(
        450 core, layout(location = 0) in vec2 v_texcoord;

        layout(location = 0) out vec4 frag_color;

        layout(binding = 0) uniform sampler2D tex;

        void main() { frag_color = texture(tex, v_texcoord); });

  public:
    ComputeShaderExample() : ctx(nullptr), window(nullptr)
    {
    }

    ~ComputeShaderExample()
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

        window = SDL_CreateWindow("MGL Compute Shader Example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width,
                                  height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
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

    bool setupTexture()
    {

        glCreateTextures(GL_TEXTURE_2D, 1, &texture);
        glTextureStorage2D(texture, 1, GL_RGBA32F, width, height);

        return true;
    }

    bool setupGeometry()
    {

        float vertices[] = {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 1.0f, 0.0f,
                            1.0f,  1.0f,  1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 1.0f};

        unsigned int indices[] = {0, 1, 2, 2, 3, 0};

        glCreateVertexArrays(1, &vao);
        glCreateBuffers(1, &vbo);
        glCreateBuffers(1, &ebo);

        glNamedBufferStorage(vbo, sizeof(vertices), vertices, GL_MAP_WRITE_BIT);
        glNamedBufferStorage(ebo, sizeof(indices), indices, GL_MAP_WRITE_BIT);

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

    GLuint time_ubo = 0;

    void setupTimeUniform()
    {

        glGenBuffers(1, &time_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, time_ubo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(float), &time, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBufferBase(GL_UNIFORM_BUFFER, 1, time_ubo);
    }

    void updateComputeShader()
    {

        glBindBuffer(GL_UNIFORM_BUFFER, time_ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(float), &time);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        static int frame_count = 0;
        if (frame_count % 60 == 0)
        {
            std::print("Frame {}, Time: {}\n", frame_count, time);
        }
        frame_count++;

        glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

        glUseProgram(compute_program);

        glDispatchCompute((width + 15) / 16, (height + 15) / 16, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glFinish();
    }

    void render()
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

    void run()
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

        setupTimeUniform();

        bool running = true;
        SDL_Event event;
        Uint32 last_time = SDL_GetTicks();

        std::print("MGL Compute Shader Example\n");
        std::print("==========================\n");
        std::print("Watch the animated circular wave patterns!\n");
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

            SDL_Delay(16);
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
    ComputeShaderExample example;

    if (!example.initialize())
    {
        std::print(stderr, "Failed to initialize example\n");
        return 1;
    }

    example.run();

    return 0;
}
