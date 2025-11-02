/**
 * Slime simulation using compute shaders
 */

#define GL_GLEXT_PROTOTYPES 1
#include <GL/glcorearb.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);

std::string readShaderFile(const char *filePath)
{
    std::ifstream shaderFile;
    std::stringstream shaderStream;

    // Open file
    shaderFile.open(filePath);
    if (!shaderFile.is_open())
    {
        std::cerr << "Failed to open shader file: " << filePath << std::endl;
        return "";
    }

    // Read file's buffer contents into stream
    shaderStream << shaderFile.rdbuf();

    // Close file
    shaderFile.close();

    // Convert stream into string
    return shaderStream.str();
}

GLuint compileShader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;
        return 0;
    }
    return shader;
}

GLuint createComputeProgram()
{
    // Load compute shader source from file
    std::string computeSource = readShaderFile("compute.glsl");

    if (computeSource.empty())
    {
        std::cerr << "Failed to load compute shader file" << std::endl;
        return 0;
    }

    GLuint computeShader = compileShader(GL_COMPUTE_SHADER, computeSource.c_str());

    if (computeShader == 0)
    {
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, computeShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Compute program linking failed:\n" << infoLog << std::endl;
        return 0;
    }

    glDeleteShader(computeShader);

    return program;
}

GLuint createDisplayProgram()
{
    // Load shader sources from files
    std::string vertSource = readShaderFile("display.vert");
    std::string fragSource = readShaderFile("display.frag");

    if (vertSource.empty() || fragSource.empty())
    {
        std::cerr << "Failed to load display shader files" << std::endl;
        return 0;
    }

    GLuint vertShader = compileShader(GL_VERTEX_SHADER, vertSource.c_str());
    GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragSource.c_str());

    if (vertShader == 0 || fragShader == 0)
    {
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Display program linking failed:\n" << infoLog << std::endl;
        return 0;
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    return program;
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    // --------------------
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;
    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Slime Simulation", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Set initial viewport
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Create compute shader program
    GLuint computeProgram = createComputeProgram();
    if (computeProgram == 0)
    {
        std::cerr << "Failed to create compute program" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Create display shader program
    GLuint displayProgram = createDisplayProgram();
    if (displayProgram == 0)
    {
        std::cerr << "Failed to create display program" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Create texture for compute shader output
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Bind texture as image for compute shader
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    // Set uniforms for compute shader
    glUseProgram(computeProgram);
    GLint widthLoc = glGetUniformLocation(computeProgram, "width");
    GLint heightLoc = glGetUniformLocation(computeProgram, "height");
    glUniform1ui(widthLoc, width);
    glUniform1ui(heightLoc, height);

    // Create fullscreen quad
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    GLuint quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    std::cout << "Slime simulation initialized" << std::endl;
    std::cout << "Window size: " << width << "x" << height << std::endl;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);

        // Dispatch compute shader
        glUseProgram(computeProgram);
        glDispatchCompute((width + 7) / 8, (height + 7) / 8, 1);

        // Wait for compute shader to finish
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // Render the texture to screen
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(displayProgram);
        glBindVertexArray(quadVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // cleanup
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteTextures(1, &texture);
    glDeleteProgram(computeProgram);
    glDeleteProgram(displayProgram);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
