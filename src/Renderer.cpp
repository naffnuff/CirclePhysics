#include "Renderer.h"

#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <functional>

#include "Shaders.h"

namespace CirclePhysics {

// Error callback
void errorCallback(int error, const char* description)
{
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

// Viewport resize callback
void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));

    // Update global window dimensions
    renderer->m_windowWidth = (float)width;
    renderer->m_windowHeight = (float)height;

    glViewport(0, 0, width, height);
}

// Key callback
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

// Utility function to compile shaders
GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Check for compilation errors
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        glfwTerminate();
        throw std::runtime_error(std::string("Shader compilation error: ") + infoLog);
    }

    return shader;
}

// Utility function to create shader program
GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource)
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // Check for linking errors
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        glfwTerminate();
        throw std::runtime_error(std::string("Shader program linking error: ") + infoLog);
    }

    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

void Renderer::intialize()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // Set error callback
    glfwSetErrorCallback(errorCallback);

    // Set window hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Enable window resizing
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    m_window = glfwCreateWindow((int)m_config.initialWindowWidth, (int)m_config.initialWindowHeight, "Circle Physics", NULL, NULL);
    if (!m_window)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwSetWindowUserPointer(m_window, this);

    // Set context
    glfwMakeContextCurrent(m_window);
    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);

    // Enable VSync to match monitor refresh rate
    glfwSwapInterval(1);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLEW");
    }

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create and compile shaders
    m_circleShaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    // Quad vertices for a unit circle (actually a square that will be made circular in the fragment shader)
    float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f 
    };

    // Indices for drawing the quad as two triangles
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    // Create buffers for circles
    glGenVertexArrays(1, &m_vertexArray);
    glGenBuffers(1, &m_vertexBuffer);
    glGenBuffers(1, &m_indexBuffer);
    glGenBuffers(1, &m_instanceBuffer);

    // Bind vertex array
    glBindVertexArray(m_vertexArray);

    // Static data

    // Setup buffer for the quad vertices
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Setup buffer for the triangle indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Instance data

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Setup buffer for the circle instances
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceBuffer);
    glBufferData(GL_ARRAY_BUFFER, m_engine.getCircleCount() * sizeof(CircleRenderData), m_engine.getRenderData(), GL_DYNAMIC_DRAW);

    // Instance-attribute layout
    
    // Position
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(CircleRenderData), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    // Previous position
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(CircleRenderData), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // Color
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(CircleRenderData), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    // Radius
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(CircleRenderData), (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    // Outline width
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(CircleRenderData), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Initialize window dimensions
    m_windowWidth = m_config.initialWindowWidth;
    m_windowHeight = m_config.initialWindowHeight;

    // Set uniforms
    m_projectionUniform = glGetUniformLocation(m_circleShaderProgram, "u_Projection");
    m_outlineCirclesUniform = glGetUniformLocation(m_circleShaderProgram, "u_OutlineCircles");
    m_interpolationFactorUniform = glGetUniformLocation(m_circleShaderProgram, "u_InterpolationFactor");
}

void Renderer::run()
{
    // FPS calculation variables
    double lastTime = glfwGetTime();
    double lastReportTime = lastTime;
    int frameCount = 0;
    int stepCount = 0;
    double accumulatedStepTime = 0.0;

    // Fixed time step variables
    double fixedTimeStep = 1.0 / m_config.physicsFrequency;
    double accumulator = 0.0;

    // Main loop
    while (!glfwWindowShouldClose(m_window))
    {
        // Frame time
        const double currentTime = glfwGetTime();
        double frameTime = currentTime - lastTime;
        lastTime = currentTime;

        // Cap the maximum frame time to avoid spiral of death
        if (frameTime > 0.25)
        {
            frameTime = 0.25;
        }

        // Accumulate time
        accumulator += frameTime;

        float scale = m_windowHeight / m_config.initialWindowHeight;

        // Update projection matrix based on current window dimensions
        const float aspectRatio = m_windowWidth / m_windowHeight;

        m_engine.setWorldBounds(scale * aspectRatio, scale);

        bool worldUpdated = false;

        // Fixed time step physics updates
        while (accumulator >= fixedTimeStep)
        {
            const double beforeStepTime = glfwGetTime();
            m_engine.step(currentTime, fixedTimeStep);
            const float stepTime = glfwGetTime() - beforeStepTime;
            if (m_config.scalePhysics && stepTime > fixedTimeStep)
            {
                // Draw down physics resolution to keep fps up
                m_config.physicsFrequency -= 1;
                fixedTimeStep = 1.0 / m_config.physicsFrequency;
            }
            accumulatedStepTime += stepTime;
            ++stepCount;
            accumulator -= fixedTimeStep;
            worldUpdated = true;
        }

        ++frameCount;

        // Print FPS to stdout every second
        if (currentTime - lastReportTime >= 1.0)
        {
            const double fps = (double)frameCount / (currentTime - lastReportTime);
            const double averageStepTime = accumulatedStepTime / stepCount;

            std::cout << std::endl;
            std::cout << "Window size: " << (int)m_windowWidth << "x" << (int)m_windowHeight << std::endl;
            std::cout << "Circle count: " << m_engine.getCircleCount() << std::endl;
            std::cout << "Average FPS: " << std::fixed << std::setprecision(1) << fps << std::endl;
            std::cout << "Physics frequency: " << m_config.physicsFrequency << " Hz (" << fixedTimeStep * 1000.0 << " ms)" << std::endl;
            if (stepCount > 0)
            {
                std::cout << "Average step time: " << std::fixed << std::setprecision(2) << averageStepTime * 1000.0 << " ms" << std::endl;
            }
            // Uncomment to compare performance of broad-phase methods (naive vs spatial partitioning)
            //std::cout << "Spatial partitioning is " << (m_engine.m_useSpatialPartitioning ? "ON" : "OFF") << std::endl;
            //m_engine.m_useSpatialPartitioning = !m_engine.m_useSpatialPartitioning;

            frameCount = 0;
            accumulatedStepTime = 0.0;
            stepCount = 0;
            lastReportTime = currentTime;
        }

        // Clear the screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Only update instance buffer when physics has been stepped or new circles added
        if (worldUpdated)
        {
            // Update circle-instance buffer with new data
            glBindBuffer(GL_ARRAY_BUFFER, m_instanceBuffer);
            glBufferData(GL_ARRAY_BUFFER, m_engine.getCircleCount() * sizeof(CircleRenderData), m_engine.getRenderData(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        // Apply shaders
        glUseProgram(m_circleShaderProgram);

        const float projection[16] = {
            1.0f / scale / aspectRatio, 0.0f,           0.0f, 0.0f,
            0.0f,                       1.0f / scale,   0.0f, 0.0f,
            0.0f,                       0.0f,           1.0f, 0.0f,
            0.0f,                       0.0f,           0.0f, 1.0f
        };

        // Calculate interpolation factor
        const float interpolationFactor = accumulator / fixedTimeStep;

        // Set uniforms
        glUniformMatrix4fv(m_projectionUniform, 1, GL_FALSE, projection);
        glUniform1i(m_outlineCirclesUniform, m_config.outlineCircles);
        glUniform1f(m_interpolationFactorUniform, interpolationFactor);

        // Draw circles
        glBindVertexArray(m_vertexArray);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, m_engine.getCircleCount());
        glBindVertexArray(0);

        // Swap buffers and poll events
        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }

}

void Renderer::cleanUp()
{
    if (m_vertexArray)
    {
        glDeleteVertexArrays(1, &m_vertexArray);
    }
    if (m_vertexBuffer)
    {
        glDeleteBuffers(1, &m_vertexBuffer);
    }
    if (m_indexBuffer)
    {
        glDeleteBuffers(1, &m_indexBuffer);
    }
    if (m_instanceBuffer)
    {
        glDeleteBuffers(1, &m_instanceBuffer);
    }
    if (m_circleShaderProgram)
    {
        glDeleteProgram(m_circleShaderProgram);
    }
}

}