/*
 * Copyright (C) Michael Larson on 1/6/2022
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * mgl_gtest.cpp
 * Complete OpenGL MGL test suite using Google Test
 *
 */

#include <gtest/gtest.h>
#include <mach/mach_vm.h>
#include <mach/mach_init.h>
#include <mach/vm_map.h>

#include <stdbool.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>
#include <vector>
#include <functional>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/glcorearb.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

extern "C"
{
#include "MGLContext.h"
}
#include "MGLRenderer.h"

// change main.c to main.cpp to use glm...
#include <glm/glm.hpp>

using glm::mat4;
using glm::vec3;
#include <glm/gtc/matrix_transform.hpp>

#define glDeleteBuffers(...)                                                                                           \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0);
#define glDeleteVertexArrays(...)                                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0);
#define glDeleteProgram(...)                                                                                           \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0);
#define glDeleteFramebuffers(...)                                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0);

#define GLSL(version, shader) "#version " #version "\n" #shader

#define DEF_PIXEL_FOR_TYPE(_type_) _type_ *pixel = (_type_ *)ptr

#define WR_NORM_PIXELR(_type_, _scale_, _r_)                                                                           \
    {                                                                                                                  \
        *pixel++ = (_type_)(_r_ * _scale_);                                                                            \
    }
#define WR_NORM_PIXELRG(_type_, _scale_, _r_, _g_)                                                                     \
    {                                                                                                                  \
        *pixel++ = (_type_)(_r_ * _scale_);                                                                            \
        *pixel++ = (_type_)(_g_ * _scale_);                                                                            \
    }
#define WR_NORM_PIXELRGB(_type_, _scale_, _r_, _g_, _b_)                                                               \
    {                                                                                                                  \
        *pixel++ = (_type_)(_r_ * _scale_);                                                                            \
        *pixel++ = (_type_)(_g_ * _scale_);                                                                            \
        *pixel++ = (_type_)(_b_ * _scale_);                                                                            \
    }
#define WR_NORM_PIXELRGBA(_type_, _scale_, _r_, _g_, _b_, _a_)                                                         \
    {                                                                                                                  \
        *pixel++ = (_type_)(_r_ * _scale_);                                                                            \
        *pixel++ = (_type_)(_g_ * _scale_);                                                                            \
        *pixel++ = (_type_)(_b_ * _scale_);                                                                            \
        *pixel++ = (_type_)(_a_ * _scale_);                                                                            \
    }

#define WR_NORM_PIXEL_FORMAT(_format_, _type_, _scale_)                                                                \
    {                                                                                                                  \
        DEF_PIXEL_FOR_TYPE(_type_);                                                                                    \
        switch (_format_)                                                                                              \
        {                                                                                                              \
        case GL_RED:                                                                                                   \
            WR_NORM_PIXELR(_type_, _scale_, r);                                                                        \
            break;                                                                                                     \
        case GL_RG:                                                                                                    \
            WR_NORM_PIXELRG(_type_, _scale_, r, g);                                                                    \
            break;                                                                                                     \
        case GL_RGB:                                                                                                   \
            WR_NORM_PIXELRGB(_type_, _scale_, r, g, b);                                                                \
            break;                                                                                                     \
        case GL_RGBA:                                                                                                  \
            WR_NORM_PIXELRGBA(_type_, _scale_, r, g, b, a);                                                            \
            break;                                                                                                     \
        default:                                                                                                       \
            assert(0);                                                                                                 \
        }                                                                                                              \
    }

#define WR_PIXELR(_type_, _r_, _scale_)                                                                                \
    {                                                                                                                  \
        *pixel++ = (_type_)_r_;                                                                                        \
    }
#define WR_PIXELRG(_type_, _r_, _g_, _scale_)                                                                          \
    {                                                                                                                  \
        *pixel++ = (_type_)_r_;                                                                                        \
        *pixel++ = (_type_)_g_;                                                                                        \
    }
#define WR_PIXELRGB(_type_, _r_, _g_, _b_, _scale_)                                                                    \
    {                                                                                                                  \
        *pixel++ = (_type_)_r_;                                                                                        \
        *pixel++ = (_type_)_g_;                                                                                        \
        *pixel++ = (_type_)_b_;                                                                                        \
    }
#define WR_PIXELRGBA(_type_, _r_, _g_, _b_, _a_, _scale_)                                                              \
    {                                                                                                                  \
        *pixel++ = (_type_)_r_;                                                                                        \
        *pixel++ = (_type_)_g_;                                                                                        \
        *pixel++ = (_type_)_b_;                                                                                        \
        *pixel++ = (_type_)_a_;                                                                                        \
    }

#define WR_PIXEL_FORMAT(_format_, _type_, _scale_)                                                                     \
    {                                                                                                                  \
        DEF_PIXEL_FOR_TYPE(_type_);                                                                                    \
        switch (_format_)                                                                                              \
        {                                                                                                              \
        case GL_RED:                                                                                                   \
            WR_PIXELR(int8_t, r, _scale_);                                                                             \
            break;                                                                                                     \
        case GL_RG:                                                                                                    \
            WR_PIXELRG(int8_t, r, g, _scale_);                                                                         \
            break;                                                                                                     \
        case GL_RGB:                                                                                                   \
            WR_PIXELRGB(int8_t, r, g, b, _scale_);                                                                     \
            break;                                                                                                     \
        case GL_RGBA:                                                                                                  \
            WR_PIXELRGBA(int8_t, r, g, b, a, _scale_);                                                                 \
            break;                                                                                                     \
        default:                                                                                                       \
            assert(0);                                                                                                 \
        }                                                                                                              \
    }

void write_pixel(GLenum format, GLenum type, void *ptr, float r, float g, float b, float a)
{
    r = glm::clamp(r, 0.0f, 1.0f);
    g = glm::clamp(g, 0.0f, 1.0f);
    b = glm::clamp(b, 0.0f, 1.0f);
    a = glm::clamp(a, 0.0f, 1.0f);

    assert(r <= 1.0f);
    assert(g <= 1.0f);
    assert(b <= 1.0f);
    assert(a <= 1.0f);

    assert(r >= 0.0f);
    assert(g >= 0.0f);
    assert(b >= 0.0f);
    assert(a >= 0.0f);

    switch (type)
    {
    case GL_UNSIGNED_BYTE: {
        WR_NORM_PIXEL_FORMAT(format, uint8_t, 255.0);
        break;
    }

    case GL_BYTE: {
        WR_NORM_PIXEL_FORMAT(format, int8_t, 127.0);
        break;
    }

    case GL_UNSIGNED_SHORT: {
        WR_NORM_PIXEL_FORMAT(format, uint16_t, (float)(2 ^ 16 - 1));
        break;
    }

    case GL_SHORT: {
        WR_NORM_PIXEL_FORMAT(format, int16_t, (float)(2 ^ 15 - 1));
        break;
    }

    case GL_UNSIGNED_INT: {
        WR_NORM_PIXEL_FORMAT(format, uint32_t, (float)(2 ^ 32 - 1));
        break;
    }

    case GL_INT: {
        WR_NORM_PIXEL_FORMAT(format, int32_t, (float)(2 ^ 31 - 1));
        break;
    }

    case GL_FLOAT: {
        WR_PIXEL_FORMAT(format, float, 1.0f);
        break;
    }

    case GL_UNSIGNED_BYTE_3_3_2:
    case GL_UNSIGNED_BYTE_2_3_3_REV:
        assert(0);

    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_5_6_5_REV:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_4_4_4_4_REV:
    case GL_UNSIGNED_SHORT_5_5_5_1:
    case GL_UNSIGNED_SHORT_1_5_5_5_REV:
        assert(0);
        break;

    case GL_UNSIGNED_INT_8_8_8_8:
        WR_NORM_PIXEL_FORMAT(format, uint8_t, 255.0f);
        break;

    case GL_UNSIGNED_INT_8_8_8_8_REV:
    case GL_UNSIGNED_INT_10_10_10_2:
    case GL_UNSIGNED_INT_2_10_10_10_REV:
        assert(0);
        break;

    default:
        assert(0);
    }
}

typedef struct RGBA_Pixel_t
{
    uint8_t r, g, b, a;
} RGBA_Pixel;

void *gen3DTexturePixels(GLenum format, GLenum type, GLuint repeat, GLuint width, GLuint height, GLint depth)
{
    GLuint pixel_size;
    size_t buffer_size;
    void *buffer;
    RGBA_Pixel *ptr;

    assert(format == GL_RGBA);
    assert(type == GL_UNSIGNED_BYTE);

    pixel_size = sizeForFormatType(format, type); //, 0);

    buffer_size = pixel_size * width;
    buffer_size *= height;
    buffer_size *= depth;

    // Allocate directly from VM because... 3d textures can be big
    kern_return_t err;
    vm_address_t buffer_data;
    err = vm_allocate((vm_map_t)mach_task_self(), (vm_address_t *)&buffer_data, buffer_size, VM_FLAGS_ANYWHERE);
    assert(err == 0);
    assert(buffer_data);

    buffer = (void *)buffer_data;

    ptr = (RGBA_Pixel *)buffer;

    float r, g, b;
    float dr, dg, db;

    dr = 1.0 / width;
    dg = 1.0 / height;
    db = 1.0 / depth;

    r = 0;
    g = 0;
    b = 0;

    for (int z = 0; z < depth; z++)
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                ptr->r = r * 255;
                ptr->g = g * 255;
                ptr->b = b * 255;
                ptr->a = 255;

                ptr++;

                r += dr;
            }

            r = 0;
            g += dg;
        }

        g = 0;
        b += db;
    }

    return buffer;
}

void HSVtoRGB(float H, float S, float V, float *r, float *g, float *b)
{
    if (H > 360 || H < 0 || S > 100 || S < 0 || V > 100 || V < 0)
    {
        return;
    }

    float s = S / 100;
    float v = V / 100;
    float C = s * v;
    float X = C * (1 - abs(fmod(H / 60.0, 2) - 1));

    if (H >= 0 && H < 60)
    {
        *r = C;
        *g = X;
        *b = 0;
    }
    else if (H >= 60 && H < 120)
    {
        *r = X;
        *g = C;
        *b = 0;
    }
    else if (H >= 120 && H < 180)
    {
        *r = 0;
        *g = C;
        *b = X;
    }
    else if (H >= 180 && H < 240)
    {
        *r = 0;
        *g = X;
        *b = C;
    }
    else if (H >= 240 && H < 300)
    {
        *r = X;
        *g = 0;
        *b = C;
    }
    else
    {
        *r = C;
        *g = 0;
        *b = X;
    }
}

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

float clamp(float a, float min, float max)
{
    a = MAX(a, min);
    a = MIN(a, max);

    return a;
}

void *genTexturePixels(GLenum format, GLenum type, GLuint repeat, GLuint width, GLuint height, GLint depth = 1,
                       GLboolean is_array = false)
{
    GLuint pixel_size;
    size_t buffer_size;
    void *buffer;
    uint8_t *ptr;

    pixel_size = sizeForFormatType(format, type); //, 0);

    buffer_size = pixel_size * width;

    buffer_size *= height;

    if (depth)
        buffer_size *= depth;

    buffer = malloc(buffer_size);
    assert(buffer);

    ptr = (uint8_t *)buffer;

    float r, g, b;
    float dr, dg, db;

    if (is_array)
    {
        r = 0.0;
        g = 0.0;
        b = 0.0;
    }
    else
    {
        r = 1.0;
        g = 1.0;
        b = 1.0;
    }

    dr = 1.0 / width;

    if (height > 1)
        dg = 1.0 / height;
    else
        dg = 0.0;

    if (depth > 1)
        db = 1.0 / depth;
    else
        db = 0.0;

    for (int z = 0; z < depth; z++)
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                r = clamp(r, 0.0f, 1.0f);
                g = clamp(g, 0.0f, 1.0f);
                b = clamp(b, 0.0f, 1.0f);

                if (y & repeat)
                {
                    if (x & repeat)
                    {
                        write_pixel(format, type, ptr, r, g, b, 1.0);
                    }
                    else
                    {
                        write_pixel(format, type, ptr, 0.0, 0.0, 0.0, 1.0);
                    }
                }
                else
                {
                    if ((x & repeat) == 0)
                    {
                        write_pixel(format, type, ptr, r, g, b, 1.0);
                    }
                    else
                    {
                        write_pixel(format, type, ptr, 0.0, 0.0, 0.0, 1.0);
                    }
                }

                if (is_array)
                {
                    r += dr;
                }

                ptr = ptr + pixel_size;
            }

            if (is_array)
            {
                r = 0.0;
                g += dg;
            }
        }

        if (is_array)
        {
            g = 0.0;
            b += db;
        }
    }

    return buffer;
}

// Test fixture class for OpenGL setup
class MGLTest : public ::testing::Test
{
  protected:
    static SDL_Window *window;
    static GLMContext glm_ctx;
    static void *renderer;
    static int width, height;
    static int wscaled, hscaled;
    static bool initialized;

    static void SetUpTestSuite()
    {
        if (initialized)
            return;

        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            FAIL() << "Failed to initialize SDL: " << SDL_GetError();
        }

        SDL_GL_LoadLibrary("/Users/sagar/Documents/MGL/build/libmgl.dylib");

        window = SDL_CreateWindow("MGL Test", 0, 0, 600, 600,
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_METAL);

        if (window == NULL)
        {
            FAIL() << "Window could not be created! SDL_Error: " << SDL_GetError();
        }

        glm_ctx = createGLMContext(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, GL_DEPTH_COMPONENT, GL_FLOAT, 0, 0);
        MGLsetCurrentContext(glm_ctx);

        SDL_SysWMinfo info;
        SDL_VERSION(&info.version);
        if (!SDL_GetWindowWMInfo(window, &info))
        {
            FAIL() << "Couldn't GetWindowWMInfo: " << SDL_GetError();
        }
        ASSERT_EQ(info.subsystem, SDL_SYSWM_COCOA);

        renderer = CppCreateMGLRendererFromContextAndBindToWindow(glm_ctx, info.info.cocoa.window);
        if (!renderer)
        {
            FAIL() << "Couldn't create MGL renderer";
        }
        SDL_SetWindowData(window, "MGLRenderer", glm_ctx);

        SDL_GL_SetSwapInterval(0);
        SDL_GetWindowSize(window, &width, &height);
        SDL_GL_GetDrawableSize(window, &wscaled, &hscaled);
        glViewport(0, 0, wscaled, hscaled);

        initialized = true;
    }

    static void TearDownTestSuite()
    {
        if (window)
        {
            SDL_DestroyWindow(window);
        }
        SDL_Quit();
        initialized = false;
    }

    void SetUp() override
    {
        // Clear any previous OpenGL errors
        while (glGetError() != GL_NO_ERROR)
        {
        }

        // Clear buffers before each test
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void TearDown() override
    {
        // Check for OpenGL errors after each test
        GLenum error = glGetError();
        EXPECT_EQ(GL_NO_ERROR, error) << "OpenGL error: " << error;
    }

    // Helper function to swap buffers
    void SwapBuffers()
    {
        MGLswapBuffers((GLMContext)SDL_GetWindowData(window, "MGLRenderer"));
    }

    // Helper function to run test for a few frames
    void RunFrames(int numFrames, std::function<void()> renderFunc)
    {
        for (int frame = 0; frame < numFrames; frame++)
        {
            renderFunc();
            SwapBuffers();

            // Process any pending events
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                // Just consume events, don't act on them
            }
        }
    }

    // Helper functions from original code
    GLuint bindDataToVBO(GLenum target, size_t size, void *ptr, GLenum usage)
    {
        GLuint vbo = 0;
        glGenBuffers(1, &vbo);
        glBindBuffer(target, vbo);
        glBufferData(target, size, ptr, usage);
        glBindBuffer(target, 0);
        return vbo;
    }

    GLuint bindVAO(GLuint vao = 0)
    {
        if (vao)
        {
            glBindVertexArray(vao);
        }
        else
        {
            GLuint new_vao;
            glCreateVertexArrays(1, &new_vao);
            glBindVertexArray(new_vao);
            return new_vao;
        }
        return vao;
    }

    void bindAttribute(GLuint index, GLuint target, GLuint vbo, GLint size, GLenum type, GLboolean normalized,
                       GLsizei stride, const void *pointer)
    {
        glEnableVertexAttribArray(index);
        glBindBuffer(target, vbo);
        glVertexAttribPointer(index, size, type, GL_FALSE, stride, pointer);
    }

    GLuint compileGLSLProgram(GLenum shader_count, ...)
    {
        va_list argp;
        va_start(argp, shader_count);
        GLuint type;
        const char *src;
        GLuint shader;

        GLuint shader_program = glCreateProgram();

        for (int i = 0; i < shader_count; i++)
        {
            type = va_arg(argp, GLuint);
            src = va_arg(argp, const char *);
            EXPECT_NE(src, nullptr);

            shader = glCreateShader(type);
            glShaderSource(shader, 1, &src, NULL);
            glCompileShader(shader);

            // Check compilation status
            GLint status;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
            if (status == GL_FALSE)
            {
                GLint logLength;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
                std::vector<char> log(logLength);
                glGetShaderInfoLog(shader, logLength, nullptr, log.data());
                ADD_FAILURE() << "Shader compilation failed: " << log.data();
                return 0;
            }

            glAttachShader(shader_program, shader);
        }

        glLinkProgram(shader_program);

        // Check link status
        GLint status;
        glGetProgramiv(shader_program, GL_LINK_STATUS, &status);
        if (status == GL_FALSE)
        {
            GLint logLength;
            glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<char> log(logLength);
            glGetProgramInfoLog(shader_program, logLength, nullptr, log.data());
            ADD_FAILURE() << "Program linking failed: " << log.data();
        }

        va_end(argp);
        return shader_program;
    }

    GLuint createTexture(GLenum target, GLsizei width, GLsizei height = 1, GLsizei depth = 1, const void *pixels = NULL,
                         GLint level = 0, GLint internalformat = GL_RGBA8, GLenum format = GL_RGBA,
                         GLenum type = GL_UNSIGNED_BYTE)
    {
        GLuint tex;

        glGenTextures(1, &tex);
        glBindTexture(target, tex);
        switch (target)
        {
        case GL_TEXTURE_1D:
            glTexImage1D(target, level, internalformat, width, 0, format, type, pixels);
            break;

        case GL_TEXTURE_2D:
            glTexImage2D(target, level, internalformat, width, height, 0, format, type, pixels);
            break;

        case GL_TEXTURE_3D:
            glTexImage3D(target, level, internalformat, width, height, depth, 0, format, type, pixels);
            break;

        case GL_TEXTURE_CUBE_MAP:
            glTexImage2D(target, level, internalformat, width, height, 0, format, type, pixels);
            break;
        }
        glBindTexture(target, 0);

        return tex;
    }

    void bufferSubData(GLenum target, GLuint buffer, GLsizei size, const void *ptr)
    {
        glBindBuffer(target, buffer);
        glBufferSubData(target, 0, size, ptr);
        glBindBuffer(target, 0);
    }

    GLuint drawToFramebuffer()
    {
        GLuint vbo = 0, vao = 0;

        float points[] = {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f};

        vbo = bindDataToVBO(GL_ARRAY_BUFFER, 8 * sizeof(float), points, GL_STATIC_DRAW);

        const char *vertex_shader = GLSL(
            450 core, layout(location = 0) in vec2 position; layout(location = 0) out vec2 texCoord;

            void main(void) {
                gl_Position = vec4(position.xy, 0.0, 1.0);
                gl_Position = sign(gl_Position);

                texCoord = (vec2(gl_Position.x, gl_Position.y) + vec2(1.0)) / vec2(2.0);
            });

        const char *fragment_shader = GLSL(
            450 core, layout(location = 0) in vec2 texCoord; layout(location = 0) out vec4 frag_colour;

            void main(void) {
                ivec2 size = ivec2(16, 16);
                float total = floor(texCoord.x * float(size.x)) + floor(texCoord.y * float(size.y));
                bool isEven = mod(total, 2.0) == 0.0;

                vec4 col1 = vec4(0.0, 0.0, 0.0, 1.0);
                vec4 col2 = vec4(1.0, 1.0, 1.0, 1.0);

                frag_colour = (isEven) ? col1 : col2;
            });

        vao = bindVAO();
        bindAttribute(0, GL_ARRAY_BUFFER, vbo, 2, GL_FLOAT, false, 0, NULL);

        GLuint shader_program =
            compileGLSLProgram(2, GL_VERTEX_SHADER, vertex_shader, GL_FRAGMENT_SHADER, fragment_shader);
        glUseProgram(shader_program);

        GLuint fbo;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        GLuint tex_attachment = createTexture(GL_TEXTURE_2D, 256, 256);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_attachment, 0);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        EXPECT_EQ(status, GL_FRAMEBUFFER_COMPLETE);

        glViewport(0, 0, 256, 256);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glClearColor(1.0, 1.0, 1.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glFinish();

        glDrawBuffer(GL_FRONT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindVertexArray(0);
        glUseProgram(0);

        // Cleanup
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        glDeleteProgram(shader_program);
        glDeleteFramebuffers(1, &fbo);

        return tex_attachment;
    }
};

// Static member definitions
SDL_Window *MGLTest::window = nullptr;
GLMContext MGLTest::glm_ctx = nullptr;
void *MGLTest::renderer = nullptr;
int MGLTest::width = 0;
int MGLTest::height = 0;
int MGLTest::wscaled = 0;
int MGLTest::hscaled = 0;
bool MGLTest::initialized = false;

// Test cases
TEST_F(MGLTest, Clear)
{
    int a = 0;
    int e = 1;

    RunFrames(20, [&]() {
        float f = (float)a / 100.0f;
        glClearColor(1.0f - f, 0.2f, f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        a += e;
        if (a >= 100)
            e = -1;
        if (a == 0)
            e = 1;
    });
}

TEST_F(MGLTest, DrawArrays)
{
    GLuint vbo = 0, vao = 0;

    const char *vertex_shader =
        GLSL(460, layout(location = 0) in vec3 position; void main() { gl_Position = vec4(position, 1.0); });

    const char *fragment_shader =
        GLSL(460, layout(location = 0) out vec4 frag_colour; void main() { frag_colour = vec4(0.5, 0.0, 0.5, 1.0); });

    float points[] = {0.0f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f};

    vbo = bindDataToVBO(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
    vao = bindVAO();

    bindAttribute(0, GL_ARRAY_BUFFER, vbo, 3, GL_FLOAT, false, 0, NULL);

    GLuint shader_program = compileGLSLProgram(2, GL_VERTEX_SHADER, vertex_shader, GL_FRAGMENT_SHADER, fragment_shader);
    glUseProgram(shader_program);

    glViewport(0, 0, wscaled, hscaled);

    RunFrames(10, [&]() {
        glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    });

    // Cleanup
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader_program);
}

TEST_F(MGLTest, DrawArraysUniformMatrix4fv)
{
    GLuint vbo = 0, vao = 0;

    const char *vertex_shader =
        GLSL(460 core, layout(location = 0) in vec3 position; void main() { gl_Position = vec4(position, 1.0); });

    const char *fragment_shader = GLSL(
        460 core, layout(location = 0) out vec4 frag_colour; layout(location = 1) uniform mat4 mp;
        void main() { frag_colour = vec4(mp[0][0], mp[0][1], mp[0][2], 1.0f); });

    float points[] = {0.0f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f};

    vbo = bindDataToVBO(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
    vao = bindVAO();

    bindAttribute(0, GL_ARRAY_BUFFER, vbo, 3, GL_FLOAT, false, 0, NULL);

    GLuint shader_program = compileGLSLProgram(2, GL_VERTEX_SHADER, vertex_shader, GL_FRAGMENT_SHADER, fragment_shader);
    glUseProgram(shader_program);

    glViewport(0, 0, wscaled, hscaled);

    GLint mp_loc = glGetUniformLocation(shader_program, "mp");
    GLfloat mp_val[16];
    int ri = 1;
    int gi = 0;
    int bi = 0;
    mp_val[0] = 0.0f;
    mp_val[1] = 0.0f;
    mp_val[2] = 0.0f;
    mp_val[3] = 1.0f;
    mp_val[4] = 0.5f;
    mp_val[5] = 0.5f;
    mp_val[6] = 0.5f;
    mp_val[7] = 1.0f;
    mp_val[8] = 0.0f;
    mp_val[9] = 0.0f;
    mp_val[10] = 0.0f;
    mp_val[11] = 1.0f;
    mp_val[12] = 0.5f;
    mp_val[13] = 0.5f;
    mp_val[14] = 0.5f;
    mp_val[15] = 1.0f;

    RunFrames(15, [&]() {
        glBindVertexArray(vao);
        glUseProgram(shader_program);
        glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUniformMatrix4fv(mp_loc, 1, false, mp_val);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        mp_val[0] += ri / 100.0f;
        mp_val[1] += gi / 100.0f;
        mp_val[2] += bi / 100.0f;

        if (mp_val[0] > 1.0f)
        {
            ri = -1;
            gi = 1;
        }
        if (mp_val[0] < 0.0f)
        {
            ri = 0;
            mp_val[0] = 0.0f;
        }
        if (mp_val[1] > 1.0f)
        {
            gi = -1;
            bi = 1;
        }
        if (mp_val[1] < 0.0f)
        {
            gi = 0;
            mp_val[1] = 0.0f;
        }
        if (mp_val[2] > 1.0f)
        {
            bi = -1;
            ri = 1;
        }
        if (mp_val[2] < 0.0f)
        {
            bi = 0;
            mp_val[2] = 0.0f;
        }
    });

    // Cleanup
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader_program);
}

TEST_F(MGLTest, DrawElements)
{
    GLuint vbo = 0, elem_vbo = 0, vao = 0;

    const char *vertex_shader =
        GLSL(460, layout(location = 0) in vec3 position; void main() { gl_Position = vec4(position, 1.0); });

    const char *fragment_shader =
        GLSL(460, layout(location = 0) out vec4 frag_colour; void main() { frag_colour = vec4(0.5, 0.0, 0.5, 1.0); });

    GLfloat points[] = {0.0f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f};
    GLuint indices[] = {0, 1, 2};

    vbo = bindDataToVBO(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
    elem_vbo = bindDataToVBO(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    vao = bindVAO();
    glVertexArrayElementBuffer(vao, elem_vbo);

    bindAttribute(0, GL_ARRAY_BUFFER, vbo, 3, GL_FLOAT, false, 0, NULL);

    GLuint shader_program = compileGLSLProgram(2, GL_VERTEX_SHADER, vertex_shader, GL_FRAGMENT_SHADER, fragment_shader);
    glUseProgram(shader_program);

    glViewport(0, 0, wscaled, hscaled);

    RunFrames(10, [&]() {
        glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
    });

    // Cleanup
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &elem_vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader_program);
}

TEST_F(MGLTest, DrawElementsVertexAttribute)
{
    GLuint vbo = 0, elem_vbo = 0, vao = 0;

    const char *vertex_shader = GLSL(
        460, layout(location = 0) in vec3 position; layout(location = 1) in vec3 col;
        layout(location = 0) out vec3 col_out; void main() {
            gl_Position = vec4(position, 1.0);
            col_out = col;
        });

    const char *fragment_shader = GLSL(
        460, layout(location = 0) in vec3 color_in; layout(location = 0) out vec4 frag_colour;
        void main() { frag_colour = vec4(color_in, 1.0); });

    GLfloat verts[] = {// pos               // col
                       0.0f, 0.5f, 0.0f, 1.0f,  0.0f,  0.0f, 0.5f, -0.5f, 0.0f,
                       0.0f, 1.0f, 0.0f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,  1.0f};

    GLuint indices[] = {0, 1, 2};

    vbo = bindDataToVBO(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    elem_vbo = bindDataToVBO(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    vao = bindVAO();
    glVertexArrayElementBuffer(vao, elem_vbo);

    bindAttribute(0, GL_ARRAY_BUFFER, vbo, 3, GL_FLOAT, false, 6 * sizeof(GLfloat), NULL);
    bindAttribute(1, GL_ARRAY_BUFFER, vbo, 3, GL_FLOAT, false, 6 * sizeof(GLfloat), (void *)(3 * sizeof(float)));

    GLuint shader_program = compileGLSLProgram(2, GL_VERTEX_SHADER, vertex_shader, GL_FRAGMENT_SHADER, fragment_shader);
    glUseProgram(shader_program);

    glViewport(0, 0, wscaled, hscaled);

    RunFrames(10, [&]() {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
    });

    // Cleanup
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &elem_vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader_program);
}

TEST_F(MGLTest, DrawRangeElements)
{
    GLuint vbo = 0, elem_vbo = 0, vao = 0;

    const char *vertex_shader =
        GLSL(460, layout(location = 0) in vec3 position; void main() { gl_Position = vec4(position, 1.0); });

    const char *fragment_shader =
        GLSL(460, layout(location = 0) out vec4 frag_colour; void main() { frag_colour = vec4(0.5, 0.0, 0.5, 1.0); });

    GLfloat points[] = {
        0.0f, 0.5f,  0.0f, 0.0f,  0.5f,  0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.0f, // start @ 3
        0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f,                                     // end @ 5
        0.0f, 0.5f,  0.0f, 0.0f,  0.5f,  0.0f, 0.0f, 0.5f, 0.0f,
    };

    GLuint indices[] = {0, 0, 0, 3, 4, 5, 0, 0};

    vbo = bindDataToVBO(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
    elem_vbo = bindDataToVBO(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    vao = bindVAO();
    glVertexArrayElementBuffer(vao, elem_vbo);

    bindAttribute(0, GL_ARRAY_BUFFER, vbo, 3, GL_FLOAT, false, 0, NULL);

    GLuint shader_program = compileGLSLProgram(2, GL_VERTEX_SHADER, vertex_shader, GL_FRAGMENT_SHADER, fragment_shader);
    glUseProgram(shader_program);

    glViewport(0, 0, wscaled, hscaled);

    RunFrames(10, [&]() {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawRangeElements(GL_TRIANGLES, 3, 5, 3, GL_UNSIGNED_INT, 0);
    });

    // Cleanup
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &elem_vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader_program);
}

TEST_F(MGLTest, DrawArraysInstanced)
{
    GLuint vbo = 0, vao = 0;

    const char *vertex_shader = GLSL(
        460, layout(location = 0) in vec3 position; layout(location = 0) out vec4 color; void main() {
            vec3 instance_position = position;
            if (gl_InstanceID == 1)
                instance_position.x += 0.1;
            else if (gl_InstanceID == 2)
                instance_position.x += 0.2;
            else if (gl_InstanceID == 3)
                instance_position.x += 0.3;
            if (gl_InstanceID == 1)
                color = vec4(1.0, 0.0, 0.0, 1.0);
            else if (gl_InstanceID == 2)
                color = vec4(0.0, 1.0, 0.0, 1.0);
            else if (gl_InstanceID == 3)
                color = vec4(0.0, 0.0, 1.0, 1.0);
            else
                color = vec4(0.5, 0.0, 0.5, 1.0);
            gl_Position = vec4(instance_position, 1.0);
        });

    const char *fragment_shader = GLSL(
        460, layout(location = 0) in vec4 color; layout(location = 0) out vec4 frag_colour;
        void main() { frag_colour = color; });

    float points[] = {0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, -0.5f, 0.0f};

    vbo = bindDataToVBO(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
    vao = bindVAO();

    bindAttribute(0, GL_ARRAY_BUFFER, vbo, 3, GL_FLOAT, false, 0, NULL);

    GLuint shader_program = compileGLSLProgram(2, GL_VERTEX_SHADER, vertex_shader, GL_FRAGMENT_SHADER, fragment_shader);
    glUseProgram(shader_program);

    glViewport(0, 0, wscaled, hscaled);

    RunFrames(10, [&]() {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 3, 4);
    });

    // Cleanup
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader_program);
}

TEST_F(MGLTest, DrawArraysInstancedDivisor)
{
    GLuint vbo[2], vao = 0;

    const char *vertex_shader = GLSL(
        460, layout(location = 0) in vec3 position; layout(location = 1) in vec4 col_in;
        layout(location = 0) out vec4 col_out; void main() {
            vec3 instance_position = position;
            if (gl_InstanceID == 1)
                instance_position.x += 0.1;
            else if (gl_InstanceID == 2)
                instance_position.x += 0.2;
            else if (gl_InstanceID == 3)
                instance_position.x += 0.3;
            gl_Position = vec4(instance_position, 1.0);
            col_out = col_in;
        });

    const char *fragment_shader = GLSL(
        460, layout(location = 0) in vec4 color; layout(location = 0) out vec4 frag_colour;
        void main() { frag_colour = color; });

    float points[] = {0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, -0.5f, 0.0f};

    float colors[] = {
        1.0f, 0.0f, 0.0f, 1.0f, // for a red, green, blue, purple triangle
        0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    };

    vbo[0] = bindDataToVBO(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
    vbo[1] = bindDataToVBO(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
    vao = bindVAO();

    bindAttribute(0, GL_ARRAY_BUFFER, vbo[0], 3, GL_FLOAT, false, 0, NULL);
    bindAttribute(1, GL_ARRAY_BUFFER, vbo[1], 4, GL_FLOAT, false, 0, NULL);

    glVertexAttribDivisor(1, 1); // fetch attribute once per instance

    GLuint shader_program = compileGLSLProgram(2, GL_VERTEX_SHADER, vertex_shader, GL_FRAGMENT_SHADER, fragment_shader);
    glUseProgram(shader_program);

    glViewport(0, 0, wscaled, hscaled);

    RunFrames(10, [&]() {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 3, 4);
    });

    // Cleanup
    glDeleteBuffers(2, vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader_program);
}

TEST_F(MGLTest, UniformBuffer)
{
    GLuint vbo = 0, ubo = 0;

    const char *vertex_shader = GLSL(
        450 core,
        layout(location = 0) in vec3 position; // attribute 0
        layout(location = 0) out vec4 color;

        layout(binding = 0) uniform vertex_data // vertex buffer 0
        { vec4 colors[16]; };

        void main() {
            gl_Position = vec4(position, 1.0);
            color = vec4(colors[0].x, colors[0].y, colors[0].z, 1.0);
        });

    const char *fragment_shader = GLSL(
        450 core, layout(location = 0) in vec4 color; layout(location = 0) out vec4 frag_colour;

        void main() { frag_colour = color; });

    float points[] = {0.0f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f};

    GLuint vao = 0;
    glCreateVertexArrays(1, &vao);
    glBindVertexArray(vao);

    vbo = bindDataToVBO(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

    struct
    {
        float x, y, z, w;
    } mptr[16];

    for (int i = 0; i < 16; i++)
    {
        mptr[i].x = 1.0f;
        mptr[i].y = mptr[i].z = mptr[i].w = 0.0f;
    }

    ubo = bindDataToVBO(GL_UNIFORM_BUFFER, sizeof(mptr), mptr, GL_STATIC_DRAW);

    bindAttribute(0, GL_ARRAY_BUFFER, vbo, 3, GL_FLOAT, false, 0, NULL);

    GLuint shader_program = compileGLSLProgram(2, GL_VERTEX_SHADER, vertex_shader, GL_FRAGMENT_SHADER, fragment_shader);
    glUseProgram(shader_program);

    GLuint matrices_loc = glGetUniformBlockIndex(shader_program, "vertex_data");
    EXPECT_EQ(matrices_loc, 0u) << "Uniform block index should be 0";

    glBindBufferBase(GL_UNIFORM_BUFFER, matrices_loc, ubo);

    glViewport(0, 0, wscaled, hscaled);

    RunFrames(10, [&]() {
        glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    });

    // Cleanup
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ubo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader_program);
}

TEST_F(MGLTest, Texture1D)
{
    GLuint vbo = 0, tex_vbo = 0, mat_ubo = 0;

    const char *vertex_shader = GLSL(
        450 core, layout(location = 0) in vec3 position; layout(location = 1) in float in_texcords;
        layout(location = 0) out float out_texcoords; layout(binding = 0) uniform matrices { mat4 rotZ; }; void main() {
            gl_Position = rotZ * vec4(position, 1.0);
            out_texcoords = in_texcords;
        });

    const char *fragment_shader = GLSL(
        450 core, layout(location = 0) in float in_texcords; layout(location = 0) out vec4 frag_colour;

        uniform sampler2D image;

        void main() {
            vec4 tex_color = texture(image, vec2(in_texcords, 1.0f));
            frag_colour = tex_color;
        });

    float points[] = {
        0.0f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f,
    };

    float texcoords[] = {
        0.0f,
        0.5f,
        1.0f,
    };

    vbo = bindDataToVBO(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
    tex_vbo = bindDataToVBO(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);

    mat4 rotZ = glm::identity<mat4>();
    float angle = M_PI / 6;
    rotZ = glm::rotate(glm::identity<mat4>(), angle, glm::vec3(0, 0, 1));

    mat_ubo = bindDataToVBO(GL_UNIFORM_BUFFER, sizeof(mat4), &rotZ[0][0], GL_STATIC_DRAW);

    GLuint vao = 0;
    glCreateVertexArrays(1, &vao);
    glBindVertexArray(vao);

    bindAttribute(0, GL_ARRAY_BUFFER, vbo, 3, GL_FLOAT, false, 0, NULL);
    bindAttribute(1, GL_ARRAY_BUFFER, tex_vbo, 1, GL_FLOAT, false, 0, NULL);

    GLuint shader_program = compileGLSLProgram(2, GL_VERTEX_SHADER, vertex_shader, GL_FRAGMENT_SHADER, fragment_shader);
    glUseProgram(shader_program);

    GLuint matrices_loc = glGetUniformBlockIndex(shader_program, "matrices");
    EXPECT_EQ(matrices_loc, 0u);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, mat_ubo);

    // generate 1d texture
    GLuint tex = createTexture(GL_TEXTURE_1D, 256, 1, 1, genTexturePixels(GL_RGBA, GL_FLOAT, 0x10, 256, 1));
    glBindTexture(GL_TEXTURE_1D, tex);

    glViewport(0, 0, wscaled, hscaled);
    glClearColor(0.0, 0.0, 0.0, 0.0);

    RunFrames(10, [&]() {
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_LINE_STRIP, 0, 4);

        angle += (M_PI / 180);
        rotZ = glm::rotate(glm::identity<mat4>(), angle, glm::vec3(0, 0, 1));
        bufferSubData(GL_UNIFORM_BUFFER, mat_ubo, sizeof(mat4), &rotZ[0][0]);
    });

    // Cleanup
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &tex_vbo);
    glDeleteBuffers(1, &mat_ubo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader_program);
    glDeleteTextures(1, &tex);
}

TEST_F(MGLTest, Texture2D)
{
    GLuint vbo = 0, col_vbo = 0, tex_vbo = 0, mat_ubo = 0, scale_ubo = 0, col_att_ubo = 0, vao = 0;

    const char *vertex_shader = GLSL(
        450 core, layout(location = 0) in vec3 position; layout(location = 1) in vec3 in_color;
        layout(location = 2) in vec2 in_texcords;

        layout(location = 0) out vec4 out_color; layout(location = 1) out vec2 out_texcoords;

        layout(binding = 0) uniform matrices { mat4 rotMatrix; };

        layout(binding = 1) uniform scale { float pos_scale; };

        void main() {
            gl_Position = rotMatrix * vec4(position, 1.0);
            out_color = vec4(in_color, 1.0);
            out_texcoords = in_texcords;
        });

    const char *fragment_shader = GLSL(
        450 core, layout(location = 0) in vec4 in_color; layout(location = 1) in vec2 in_texcords;

        layout(location = 0) out vec4 frag_colour;

        layout(binding = 2) uniform color_att { float att; };

        uniform sampler2D image;

        void main() {
            vec4 tex_color = texture(image, in_texcords);
            frag_colour = in_color * att * tex_color;
        });

    float points[] = {0.0f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f};

    float color[] = {
        1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    };

    float texcoords[] = {
        0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    };

    vbo = bindDataToVBO(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
    col_vbo = bindDataToVBO(GL_ARRAY_BUFFER, sizeof(color), color, GL_STATIC_DRAW);
    tex_vbo = bindDataToVBO(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);

    mat4 rotZ = glm::identity<mat4>();
    float angle = M_PI / 6;
    rotZ = glm::rotate(glm::identity<mat4>(), angle, glm::vec3(0, 0, 1));

    mat_ubo = bindDataToVBO(GL_UNIFORM_BUFFER, sizeof(mat4), &rotZ[0][0], GL_STATIC_DRAW);

    float scale = -1.0;
    scale_ubo = bindDataToVBO(GL_ARRAY_BUFFER, sizeof(scale), &scale, GL_STATIC_DRAW);

    float att = 1.0;
    col_att_ubo = bindDataToVBO(GL_ARRAY_BUFFER, sizeof(att), &att, GL_STATIC_DRAW);

    vao = bindVAO();

    bindAttribute(0, GL_ARRAY_BUFFER, vbo, 3, GL_FLOAT, false, 0, NULL);
    bindAttribute(1, GL_ARRAY_BUFFER, col_vbo, 3, GL_FLOAT, false, 0, NULL);
    bindAttribute(2, GL_ARRAY_BUFFER, tex_vbo, 2, GL_FLOAT, false, 0, NULL);

    // clear currently bound buffer
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint shader_program = compileGLSLProgram(2, GL_VERTEX_SHADER, vertex_shader, GL_FRAGMENT_SHADER, fragment_shader);
    glUseProgram(shader_program);

    GLuint matrices_loc = glGetUniformBlockIndex(shader_program, "matrices");
    EXPECT_EQ(matrices_loc, 0u);

    GLuint scale_loc = glGetUniformBlockIndex(shader_program, "scale");
    EXPECT_EQ(scale_loc, 1u);

    GLuint color_att_loc = glGetUniformBlockIndex(shader_program, "color_att");
    EXPECT_EQ(color_att_loc, 2u);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, mat_ubo);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, scale_ubo);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, col_att_ubo);

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 genTexturePixels(GL_RGBA, GL_UNSIGNED_BYTE, 0x10, 256, 256));

    glViewport(0, 0, wscaled, hscaled);
    glClearColor(0.2f, 0.2f, 0.2f, 0.0f);

    float att_delta = 0.01f;

    RunFrames(15, [&]() {
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        angle += (M_PI / 180);
        rotZ = glm::rotate(glm::identity<mat4>(), angle, glm::vec3(0, 0, 1));

        glBindBuffer(GL_UNIFORM_BUFFER, mat_ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), &rotZ[0][0]);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        att += att_delta;
        if (att > 1.0)
        {
            att = 1.0;
            att_delta *= -1.0;
        }
        else if (att < 0.0)
        {
            att = 0.0;
            att_delta *= -1.0;
        }

        glBindBuffer(GL_UNIFORM_BUFFER, col_att_ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(float), &att);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    });

    // Cleanup
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &col_vbo);
    glDeleteBuffers(1, &tex_vbo);
    glDeleteBuffers(1, &mat_ubo);
    glDeleteBuffers(1, &scale_ubo);
    glDeleteBuffers(1, &col_att_ubo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader_program);
    glDeleteTextures(1, &tex);
}

TEST_F(MGLTest, Texture3D)
{
    GLuint vbo = 0, tex_vbo = 0, mat_ubo = 0;

    const char *vertex_shader = GLSL(
        450 core, layout(location = 0) in vec3 position; layout(location = 1) in vec3 in_texcords;

        layout(location = 0) out vec3 out_texcoords;

        layout(binding = 0) uniform matrices { mat4 mvp; };

        void main() {
            gl_Position = mvp * vec4(position, 1.0);
            out_texcoords = in_texcords;
        });

    const char *fragment_shader = GLSL(
        450 core, layout(location = 0) in vec3 in_texcords;

        layout(location = 0) out vec4 frag_colour;

        uniform sampler3D image;

        void main() {
            vec4 tex_color = texture(image, in_texcords);
            frag_colour = tex_color;
        });

    float points[] = {-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,
                      -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,
                      1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,
                      -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,
                      1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,
                      1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
                      1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,
                      1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  -1.0f, 1.0f};

    float texcoords[] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                         1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
                         0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
                         0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
                         1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
                         1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
                         0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f};

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferStorage(GL_ARRAY_BUFFER, 36 * 3 * sizeof(float), points, GL_MAP_WRITE_BIT);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glCreateBuffers(1, &tex_vbo);
    glNamedBufferStorage(tex_vbo, 36 * 3 * sizeof(float), texcoords, GL_MAP_WRITE_BIT);

    float angle = M_PI / 6;

    // Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    glm::mat4 Projection = glm::perspective(glm::radians(60.0f), (float)wscaled / (float)hscaled, 0.1f, 100.0f);

    constexpr int _A = 6;
    // Camera matrix
    glm::mat4 View = glm::lookAt(glm::vec3(_A / 4, _A / 2, _A), // Camera is at (1.5,3,6), in World Space
                                 glm::vec3(0, 0, 0),            // and looks at the origin
                                 glm::vec3(0, 1, 0)             // Head is up (set to 0,-1,0 to look upside-down)
    );

    // Model matrix : an identity matrix (model will be at the origin)
    glm::mat4 Model = glm::mat4(1.0f);

    // Our ModelViewProjection : multiplication of our 3 matrices
    glm::mat4 mvp = Projection * View * Model; // Remember, matrix multiplication is the other way around

    glCreateBuffers(1, &mat_ubo);
    glNamedBufferStorage(mat_ubo, sizeof(mat4), &mvp[0][0], GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);

    GLuint vao = 0;
    glCreateVertexArrays(1, &vao);

    glVertexArrayVertexBuffer(vao, 0, vbo, 0, 3 * sizeof(float));
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, 0, 0);
    glVertexArrayAttribBinding(vao, 0, 0);
    glEnableVertexArrayAttrib(vao, 0);

    glVertexArrayVertexBuffer(vao, 1, tex_vbo, 0, 3 * sizeof(float));
    glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, 0, 0);
    glVertexArrayAttribBinding(vao, 1, 1);
    glEnableVertexArrayAttrib(vao, 1);

    glBindVertexArray(vao);

    GLuint shader_program = compileGLSLProgram(2, GL_VERTEX_SHADER, vertex_shader, GL_FRAGMENT_SHADER, fragment_shader);
    glUseProgram(shader_program);

    GLuint matrices_loc = glGetUniformBlockIndex(shader_program, "matrices");
    EXPECT_EQ(matrices_loc, 0u);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, mat_ubo);

    constexpr int _3d_size = 64; // Smaller size for faster test
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_3D, tex);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, _3d_size, _3d_size, _3d_size, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 gen3DTexturePixels(GL_RGBA, GL_UNSIGNED_BYTE, 0x10, _3d_size, _3d_size, _3d_size));

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glViewport(0, 0, wscaled, hscaled);
    glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    RunFrames(10, [&]() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        angle += (M_PI / 360);
        Model = glm::rotate(glm::identity<mat4>(), angle, glm::vec3(1, 0, 0));
        Model = glm::rotate(Model, angle * 2, glm::vec3(0, 1, 0));
        Model = glm::rotate(Model, angle * 4, glm::vec3(0, 0, 1));

        mvp = Projection * View * Model;
        glNamedBufferSubData(mat_ubo, 0, sizeof(mat4), &mvp[0][0]);
    });

    // Cleanup
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &tex_vbo);
    glDeleteBuffers(1, &mat_ubo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader_program);
    glDeleteTextures(1, &tex);
}

TEST_F(MGLTest, TextureArray2D)
{
    GLuint vbo = 0, ebo = 0, tex_vbo = 0, mat_ubo = 0;

    const char *vertex_shader = GLSL(
        450 core, layout(location = 0) in vec2 in_position; layout(location = 1) in vec2 in_texcords;

        layout(location = 0) out vec2 out_texcoords; layout(location = 1) flat out int out_instanceID;

        layout(binding = 0) uniform matrices { mat4 mvp; };

        void main() {
            float z = gl_InstanceID * -0.5;
            gl_Position = mvp * vec4(in_position, z, 1.0);
            out_texcoords = in_texcords;
            out_instanceID = gl_InstanceID;
        });

    const char *fragment_shader = GLSL(
        450 core, layout(location = 0) in vec2 in_texcords; layout(location = 1) flat in int in_instanceID;

        layout(location = 0) out vec4 frag_colour;

        uniform sampler2DArray image;

        void main() {
            vec4 tex_color = texture(image, vec3(in_texcords, in_instanceID));
            frag_colour = tex_color;
        });

    float points[] = {
        -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f,
    };

    unsigned short elements[] = {0, 1, 3, 2};

    float texcoords[] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f};

    glCreateBuffers(1, &vbo);
    glNamedBufferStorage(vbo, sizeof(points), points, GL_MAP_WRITE_BIT);

    glCreateBuffers(1, &ebo);
    glNamedBufferStorage(ebo, sizeof(elements), elements, GL_MAP_WRITE_BIT);

    glCreateBuffers(1, &tex_vbo);
    glNamedBufferStorage(tex_vbo, sizeof(texcoords), texcoords, GL_MAP_WRITE_BIT);

    float angle = M_PI / 6;

    glm::mat4 Projection = glm::perspective(glm::radians(60.0f), (float)wscaled / (float)hscaled, 0.1f, 100.0f);

    constexpr int _A = 4;
    glm::mat4 View = glm::lookAt(glm::vec3(_A / 4, _A / 2, _A), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    glm::mat4 Model = glm::mat4(1.0f);
    glm::mat4 mvp = Projection * View * Model;

    glCreateBuffers(1, &mat_ubo);
    glNamedBufferStorage(mat_ubo, sizeof(mat4), &mvp[0][0], GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);

    GLuint vao = 0;
    glCreateVertexArrays(1, &vao);

    glVertexArrayVertexBuffer(vao, 0, vbo, 0, 2 * sizeof(float));
    glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, 0, 0);
    glVertexArrayAttribBinding(vao, 0, 0);
    glEnableVertexArrayAttrib(vao, 0);

    glVertexArrayVertexBuffer(vao, 1, tex_vbo, 0, 2 * sizeof(float));
    glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, 0, 0);
    glVertexArrayAttribBinding(vao, 1, 1);
    glEnableVertexArrayAttrib(vao, 1);

    glVertexArrayElementBuffer(vao, ebo);
    glBindVertexArray(vao);

    GLuint shader_program = compileGLSLProgram(2, GL_VERTEX_SHADER, vertex_shader, GL_FRAGMENT_SHADER, fragment_shader);
    glUseProgram(shader_program);

    GLuint matrices_loc = glGetUniformBlockIndex(shader_program, "matrices");
    EXPECT_EQ(matrices_loc, 0u);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, mat_ubo);

    constexpr int _2d_array_size = 64; // Smaller size for faster test
    constexpr int _2d_array_depth = 4; // Fewer layers for faster test

    GLuint tex;
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex);

    // test tex storage path
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, _2d_array_size, _2d_array_size, _2d_array_depth);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // test subimage path
    GLuint *pixels = (GLuint *)genTexturePixels(GL_RGBA, GL_UNSIGNED_BYTE, 0x08, _2d_array_size, _2d_array_size,
                                                _2d_array_depth, true);

    size_t image_size = _2d_array_size * _2d_array_size; // image size in pixels

    // and test pbo unpack path
    GLuint pbo;
    glCreateBuffers(1, &pbo);
    glNamedBufferStorage(pbo, image_size * _2d_array_depth * sizeof(uint32_t), pixels, GL_MAP_WRITE_BIT);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);

    for (int i = 0; i < _2d_array_depth; i++)
    {
        size_t offset = i * image_size * 4;
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, _2d_array_size, _2d_array_size, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                        (void *)offset);
    }

    glViewport(0, 0, wscaled, hscaled);
    glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    RunFrames(10, [&]() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawElementsInstanced(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, _2d_array_depth);

        angle += (M_PI / 360);
        Model = glm::rotate(glm::identity<mat4>(), angle, glm::vec3(0, 0, 1));
        mvp = Projection * View * Model;
        glNamedBufferSubData(mat_ubo, 0, sizeof(mat4), &mvp[0][0]);
    });

    // Cleanup
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &tex_vbo);
    glDeleteBuffers(1, &mat_ubo);
    glDeleteBuffers(1, &pbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader_program);
    glDeleteTextures(1, &tex);
    free(pixels);
}

TEST_F(MGLTest, ComputeShader)
{
    const char *compute_shader = GLSL(
        450 core, layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

        layout(rgba8, binding = 0) uniform image2D img_output;

        // simple cosine palette
        vec3 palette(float t) { return 0.5 + 0.5 * cos(6.2831853 * (vec3(0.0, 0.33, 0.67) + t)); }

        void main() {
            ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
            ivec2 size = imageSize(img_output);

            if (coord.x >= size.x || coord.y >= size.y)
                return;

            // normalized coords in [-1,1] with aspect correction
            vec2 p = (vec2(coord) / vec2(size)) * 2.0 - 1.0;
            float aspect = float(size.x) / float(size.y);
            p.x *= aspect;

            // Mandelbrot parameters
            vec2 c = vec2(-0.75, 0.0) + p * 1.5; // center and zoom
            vec2 z = vec2(0.0);
            const int maxIter = 300;

            int i;
            for (i = 0; i < maxIter; ++i)
            {
                // z = z^2 + c
                z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
                if (dot(z, z) > 4.0)
                    break; // escaped
            }

            vec3 color;
            if (i == maxIter)
            {
                color = vec3(0.0); // inside set = black
            }
            else
            {
                // smooth iteration count
                float mu = float(i) + 1.0 - log(log(length(z))) / log(2.0);
                float t = clamp(mu / float(maxIter), 0.0, 1.0);
                color = palette(t);
            }

            imageStore(img_output, coord, vec4(color, 1.0));
        });

    GLuint compute_program;
    compute_program = compileGLSLProgram(1, GL_COMPUTE_SHADER, compute_shader);
    assert(compute_program);

    GLuint sbo;
    glCreateBuffers(1, &sbo);
    glNamedBufferStorage(sbo, 1024 * sizeof(float), NULL, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_CLIENT_STORAGE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sbo);

    GLuint pbo;
    GLuint cell_end_y;
    cell_end_y = hscaled / 2;
    glCreateBuffers(1, &pbo);
    glNamedBufferStorage(pbo, sizeof(GLuint), &cell_end_y, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_CLIENT_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, pbo);

    GLuint abo;
    glCreateBuffers(1, &abo);
    glNamedBufferStorage(abo, sizeof(GLuint), NULL, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_CLIENT_STORAGE_BIT);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, abo);

    GLuint sampler;
    glGenSamplers(1, &sampler);

    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // dimensions of the image
    int tex_w = 256, tex_h = 256;
    GLuint textures[2];
    glCreateTextures(GL_TEXTURE_2D, 2, textures);
    for (int i = 0; i < 2; i++)
    {
        glTextureStorage2D(textures[i], 1, GL_RGBA8, tex_w, tex_h);
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_w, tex_h, GL_RGBA, GL_UNSIGNED_BYTE,
                        genTexturePixels(GL_RGBA, GL_UNSIGNED_BYTE, 0x10, tex_w, tex_h));
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindImageTexture(i, textures[i], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    }

    glUseProgram(compute_program);
    glDispatchCompute((GLuint)tex_w, (GLuint)tex_h, 2);

    // make sure writing to image has finished before read
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glFinish();

    GLuint vbo = 0, ebo = 0, tex_vbo = 0, mat_ubo = 0;

    const char *vertex_shader = GLSL(
        450 core, layout(location = 0) in vec3 position; layout(location = 1) in vec2 in_texcords;

        layout(location = 0) out vec2 out_texcoords;

        layout(binding = 0) uniform matrices { mat4 mvp; };

        void main() {
            gl_Position = mvp * vec4(position, 1.0);
            out_texcoords = in_texcords;
        });

    const char *fragment_shader = GLSL(
        450 core, layout(location = 0) in vec2 in_texcords;

        layout(location = 0) out vec4 frag_colour;

        uniform sampler2D image;

        void main() {
            vec4 tex_color = texture(image, in_texcords);
            frag_colour = tex_color;
        });

    float points[] = {
        -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f,
    };

    unsigned short elements[] = {0, 1, 3, 2};

    float texcoords[] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f};

    glCreateBuffers(1, &vbo);
    glNamedBufferStorage(vbo, sizeof(points), points, GL_MAP_WRITE_BIT);

    glCreateBuffers(1, &ebo);
    glNamedBufferStorage(ebo, sizeof(elements), elements, GL_MAP_WRITE_BIT);

    glCreateBuffers(1, &tex_vbo);
    glNamedBufferStorage(tex_vbo, sizeof(points), texcoords, GL_MAP_WRITE_BIT);

    float angle = M_1_PI / 6;

    glm::mat4 Projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 100.0f);
    constexpr int _A = 1;
    // Camera matrix
    glm::mat4 View = glm::lookAt(glm::vec3(_A, _A, _A), // Camera is at (10,10,10), in World Space
                                 glm::vec3(0, 0, 0),    // and looks at the origin
                                 glm::vec3(0, 0, 1)     // Head is up (set to 0,-1,0 to look upside-down)
    );

    // Model matrix : an identity matrix (model will be at the origin)
    glm::mat4 Model = glm::mat4(1.0f);

    // Our ModelViewProjection : multiplication of our 3 matrices
    glm::mat4 mvp = Projection * View * Model; // Remember, matrix multiplication is the other way around

    glCreateBuffers(1, &mat_ubo);
    glNamedBufferStorage(mat_ubo, sizeof(mat4), &mvp[0][0], GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);

    GLuint vao = 0;
    glCreateVertexArrays(1, &vao);

    glVertexArrayVertexBuffer(vao, 0, vbo, 0, 2 * sizeof(float));
    glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, 0, 0);
    glVertexArrayAttribBinding(vao, 0, 0);
    glEnableVertexArrayAttrib(vao, 0);

    glVertexArrayElementBuffer(vao, ebo);

    glVertexArrayVertexBuffer(vao, 1, tex_vbo, 0, 2 * sizeof(float));
    glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, 0, 0);
    glVertexArrayAttribBinding(vao, 1, 1);
    glEnableVertexArrayAttrib(vao, 1);

    glBindVertexArray(vao);

    GLuint shader_program;
    shader_program = compileGLSLProgram(2, GL_VERTEX_SHADER, vertex_shader, GL_FRAGMENT_SHADER, fragment_shader);
    glUseProgram(shader_program);

    GLuint matrices_loc = glGetUniformBlockIndex(shader_program, "matrices");
    assert(matrices_loc == 0);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, mat_ubo);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textures[0]);

    glViewport(0, 0, wscaled, hscaled);

    glClearColor(0.2, 0.2, 0.2, 0.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    RunFrames(60, [&]() {
        glUseProgram(shader_program);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, mat_ubo);
        // glBindTexture(GL_TEXTURE_2D, textures[0]);
        // glBindSampler(GL_TEXTURE0 + 1, sampler);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);

        angle += (M_PI / 360) * 0.25;
        Model = glm::rotate(glm::identity<mat4>(), angle, glm::vec3(0, 0, 1));
        mvp = Projection * View * Model;
        glNamedBufferSubData(mat_ubo, 0, sizeof(mat4), &mvp[0][0]);
        glFlush();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sbo);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, pbo);
        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, abo);

        for (int i = 0; i < 2; i++)
        {
            glBindSampler(GL_TEXTURE0 + i, sampler);
            glBindImageTexture(i, textures[i], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        }
        glBindSampler(GL_TEXTURE0, 0);
        glUseProgram(compute_program);
        glDispatchCompute((GLuint)tex_w, (GLuint)tex_h, 2);

        // make sure writing to image has finished before read
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glFlush();
    });
}

TEST_F(MGLTest, SimpleComputeShader)
{
    const char *simple_compute_shader = GLSL(
        450 core, layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

        layout(rgba32f, binding = 0) uniform image2D output_image;

        void main() {
            ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
            ivec2 size = imageSize(output_image);

            if (coord.x >= size.x || coord.y >= size.y)
                return;

            // Simple pattern: create a checkerboard pattern
            float r = ((coord.x / 4) % 2 == (coord.y / 4) % 2) ? 1.0 : 0.0;
            float g = 0.5;
            float b = float(coord.x) / float(size.x);
            float a = 1.0;

            // Write output
            imageStore(output_image, coord, vec4(r, g, b, a));
        });

    // Compile compute shader
    GLuint program = compileGLSLProgram(1, GL_COMPUTE_SHADER, simple_compute_shader);
    ASSERT_TRUE(program) << "Failed to compile simple compute shader";

    // Create output texture
    const int TEX_SIZE = 32;
    GLuint texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureStorage2D(texture, 1, GL_RGBA32F, TEX_SIZE, TEX_SIZE);

    // Bind texture to image unit
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    // Use program and dispatch compute
    glUseProgram(program);
    glDispatchCompute(TEX_SIZE / 16, TEX_SIZE / 16, 1);

    // Wait for completion
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // If we get here without crashing, the compute shader compiled and ran successfully
    std::cout << "Simple compute shader executed successfully" << std::endl;

    // Cleanup
    glDeleteTextures(1, &texture);
    glDeleteProgram(program);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}