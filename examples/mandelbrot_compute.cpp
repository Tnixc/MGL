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

const int WIDTH = 1024;
const int HEIGHT = 768;

const char *compute_source = GLSL(
    450 core, layout(local_size_x = 16, local_size_y = 16) in;
    layout(rgba32f, binding = 0) uniform writeonly image2D output_image;

    vec3 palette(float t) {
        t = pow(t, 0.3);
        vec3 bw = vec3(t);

        float boundary = smoothstep(0.7, 0.95, t);

        vec3 blue = vec3(0.1, 0.3, 0.8);
        vec3 yellow = vec3(1.0, 0.85, 0.1);
        vec3 edge_color = mix(blue, yellow, smoothstep(0.75, 0.9, t));

        return mix(bw, edge_color, boundary * 1.5);
    } void main() {
        ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
        ivec2 dims = imageSize(output_image);

        if (coord.x >= dims.x || coord.y >= dims.y)
            return;

        vec2 uv = vec2(coord) / vec2(dims) - 0.5;
        uv.x *= float(dims.x) / float(dims.y);
        uv *= 2.5;
        uv.x -= 0.5;

        vec2 z = vec2(0.0);
        int iterations = 0;
        const int max_iter = 512;

        for (int i = 0; i < max_iter; i++)
        {
            z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + uv;

            if (dot(z, z) > 4.0)
            {
                iterations = i;
                break;
            }
        }

        vec3 color;
        if (iterations < max_iter)
        {
            float smooth_iter = float(iterations) - log2(log2(dot(z, z))) + 4.0;
            float t = smooth_iter / float(max_iter);
            color = palette(t);
        }
        else
        {
            color = vec3(0.0);
        }

        imageStore(output_image, coord, vec4(color, 1.0));
    });

const char *vertex_source = GLSL(
    450 core, layout(location = 0) in vec2 position; layout(location = 1) in vec2 texcoord;
    layout(location = 0) out vec2 v_texcoord;

    void main() {
        gl_Position = vec4(position, 0.0, 1.0);
        v_texcoord = texcoord;
    });

const char *fragment_source = GLSL(
    450 core, layout(location = 0) in vec2 v_texcoord; layout(location = 0) out vec4 frag_color;
    layout(binding = 0) uniform sampler2D tex;

    void main() { frag_color = texture(tex, v_texcoord); });

int main(int argc, char **argv)
{

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::print(stderr, "SDL init failed: {}\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Mandelbrot", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

    if (!window)
    {
        std::print(stderr, "Window creation failed\n");
        return 1;
    }

    GLMContext ctx = createGLMContext(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, GL_DEPTH_COMPONENT, GL_FLOAT, 0, 0);
    MGLsetCurrentContext(ctx);

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    SDL_GetWindowWMInfo(window, &info);
    void *renderer = CppCreateMGLRendererFromContextAndBindToWindow(ctx, info.info.cocoa.window);

    GLuint compute_shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute_shader, 1, &compute_source, NULL);
    glCompileShader(compute_shader);

    GLuint compute_program = glCreateProgram();
    glAttachShader(compute_program, compute_shader);
    glLinkProgram(compute_program);
    glDeleteShader(compute_shader);

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_source, NULL);
    glCompileShader(vertex_shader);

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_source, NULL);
    glCompileShader(fragment_shader);

    GLuint render_program = glCreateProgram();
    glAttachShader(render_program, vertex_shader);
    glAttachShader(render_program, fragment_shader);
    glLinkProgram(render_program);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    GLuint texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureStorage2D(texture, 1, GL_RGBA32F, WIDTH, HEIGHT);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    float vertices[] = {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 1.0f, 0.0f,
                        1.0f,  1.0f,  1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 1.0f};

    unsigned int indices[] = {0, 1, 2, 2, 3, 0};

    GLuint vao, vbo, ebo;
    glCreateVertexArrays(1, &vao);
    glCreateBuffers(1, &vbo);
    glCreateBuffers(1, &ebo);

    glNamedBufferData(vbo, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glNamedBufferData(ebo, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexArrayVertexBuffer(vao, 0, vbo, 0, 4 * sizeof(float));
    glVertexArrayElementBuffer(vao, ebo);

    glEnableVertexArrayAttrib(vao, 0);
    glEnableVertexArrayAttrib(vao, 1);
    glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
    glVertexArrayAttribBinding(vao, 0, 0);
    glVertexArrayAttribBinding(vao, 1, 0);

    int drawable_width, drawable_height;
    SDL_GL_GetDrawableSize(window, &drawable_width, &drawable_height);
    glViewport(0, 0, drawable_width, drawable_height);

    GLuint groups_x = (WIDTH + 15) / 16;
    GLuint groups_y = (HEIGHT + 15) / 16;
    GLuint total_workgroups = groups_x * groups_y;
    GLuint total_threads = total_workgroups * 16 * 16;

    glUseProgram(compute_program);
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glDispatchCompute(groups_x, groups_y, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glFinish();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUseProgram(render_program);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    MGLswapBuffers(ctx);

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
            {
                running = false;
            }
        }
        SDL_Delay(16);
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteTextures(1, &texture);
    glDeleteProgram(compute_program);
    glDeleteProgram(render_program);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
