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
#include <vector>
#include <cmath>
#include <random>

struct Agent {
    float position[2];
    float angle;
    float padding; // For alignment
};

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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);  // Disable resizing

    // glfw window creation
    // --------------------
    const int RENDER_WIDTH = 400;
    const int RENDER_HEIGHT = 300;
    const int WINDOW_WIDTH = RENDER_WIDTH * 2;  // 800
    const int WINDOW_HEIGHT = RENDER_HEIGHT * 2;  // 600
    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Slime Simulation", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Set initial viewport to match window
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    // Use fixed render resolution regardless of window/framebuffer size
    const int width = RENDER_WIDTH;
    const int height = RENDER_HEIGHT;

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

    // Simulation parameters
    const unsigned int NUM_AGENTS = 1000;
    const float MOVE_SPEED = 50.0f;

    // Initialize agents with random positions and angles
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159265359f);

    std::vector<Agent> agents(NUM_AGENTS);
    // Use intuitive coordinate system: (0,0) at top-left, (width, height) at bottom-right
    float centerX = static_cast<float>(width) / 2.0f;
    float centerY = static_cast<float>(height) / 2.0f;

    std::cout << "DEBUG: Spawning agents at center (" << centerX << ", " << centerY << ")" << std::endl;
    std::cout << "DEBUG: Using top-left origin coordinate system" << std::endl;
    std::cout << "DEBUG: Render dimensions: " << width << " x " << height << std::endl;

    for (unsigned int i = 0; i < NUM_AGENTS; i++)
    {
        agents[i].position[0] = centerX;
        agents[i].position[1] = centerY;
        agents[i].angle = angleDist(gen);
        agents[i].padding = 0.0f;
    }

    std::cout << "DEBUG: First 3 agents initialized:" << std::endl;
    for (int i = 0; i < 3; i++) {
        std::cout << "  Agent[" << i << "]: pos=(" << agents[i].position[0] << ", " << agents[i].position[1]
                  << "), angle=" << agents[i].angle << std::endl;
    }

    // Create agent buffer
    GLuint agentBuffer;
    glGenBuffers(1, &agentBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, agentBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, agents.size() * sizeof(Agent), agents.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, agentBuffer);

    // Create texture for trail map using DSA (like the working example)
    GLuint texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);
    glTextureStorage2D(texture, 1, GL_RGBA32F, width, height);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Initialize texture to black (prevents magenta/uninitialized data)
    std::vector<float> blackPixels(width * height * 4, 0.0f);
    glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, blackPixels.data());

    // Set uniforms for compute shader
    glUseProgram(computeProgram);
    glUniform1ui(0, width);  // location 0
    glUniform1ui(1, height); // location 1
    glUniform1ui(2, NUM_AGENTS); // location 2
    glUniform1f(3, MOVE_SPEED); // location 3

    // Create fullscreen quad
    float quadVertices[] = {
        // positions   // texCoords (V flipped to match intuitive coordinate system)
        -1.0f,  1.0f,  0.0f, 0.0f,
        -1.0f, -1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 1.0f,

        -1.0f,  1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 0.0f
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

    // Set display shader uniforms
    glUseProgram(displayProgram);
    GLint texLoc = glGetUniformLocation(displayProgram, "screenTexture");
    glUniform1i(texLoc, 0); // Texture unit 0

    std::cout << "Slime simulation initialized" << std::endl;
    std::cout << "Window size: " << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << std::endl;
    std::cout << "Framebuffer size: " << fbWidth << "x" << fbHeight << std::endl;
    std::cout << "Render resolution: " << width << "x" << height << " (scaled " << (WINDOW_WIDTH/width) << "x)" << std::endl;
    std::cout << "Number of agents: " << NUM_AGENTS << std::endl;

    // Timing
    float lastFrame = 0.0f;
    float deltaTime = 0.0f;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // Calculate delta time
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // Bind texture as image and dispatch compute shader (one thread per agent)
        glUseProgram(computeProgram);
        glUniform1f(4, deltaTime); // Update deltaTime
        glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        glDispatchCompute((NUM_AGENTS + 15) / 16, 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
        glFinish();

        // Clear the screen framebuffer
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw fullscreen quad displaying the texture
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
    glDeleteBuffers(1, &agentBuffer);
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
