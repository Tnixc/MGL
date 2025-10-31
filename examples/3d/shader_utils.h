#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#define GL_GLEXT_PROTOTYPES 1
#include <GL/glcorearb.h>

#define GLSL(version, shader) "#version " #version "\n" #shader

GLuint compileGLSLProgram(int count, ...);

#endif // SHADER_UTILS_H
