#include "shader_utils.h"
#include <print>
#include <cstdarg>

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
