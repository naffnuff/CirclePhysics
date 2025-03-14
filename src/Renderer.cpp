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

// Mouse button callback
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            // Get current mouse position
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            std::cout << xpos << ":" << ypos << std::endl;
        }
        else if (action == GLFW_RELEASE)
        {
        }
    }
}

// Cursor position callback
void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
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

void Renderer::initialize()
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
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetCursorPosCallback(m_window, cursorPositionCallback);
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

    // Quad vertices for a unit circle
    float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    // Indices for the quad
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    // Create buffers for circles
    glGenVertexArrays(1, &m_vertexArray);

    // For the static data
    glGenBuffers(1, &m_vertexBuffer);
    glGenBuffers(1, &m_indexBuffer);

    // For each instance attribute
    glGenBuffers(1, &m_positionXBuffer);
    glGenBuffers(1, &m_positionYBuffer);
    glGenBuffers(1, &m_prevPositionXBuffer);
    glGenBuffers(1, &m_prevPositionYBuffer);
    glGenBuffers(1, &m_redBuffer);
    glGenBuffers(1, &m_greenBuffer);
    glGenBuffers(1, &m_blueBuffer);
    glGenBuffers(1, &m_radiusBuffer);
    glGenBuffers(1, &m_outlineWidthBuffer);

    // Bind vertex array
    glBindVertexArray(m_vertexArray);

    // Setup quad vertices and indices

    // Setup the buffer for the base quad vertices
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute for base quad
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Setup buffer for the triangle indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Setup buffers for instance attributes

    const CircleData& circleData = m_engine.getCircleData();

    // Current position X
    glBindBuffer(GL_ARRAY_BUFFER, m_positionXBuffer);
    glBufferData(GL_ARRAY_BUFFER, circleData.getCircleCount() * sizeof(float), circleData.positionsX.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    // Current position Y
    glBindBuffer(GL_ARRAY_BUFFER, m_positionYBuffer);
    glBufferData(GL_ARRAY_BUFFER, circleData.getCircleCount() * sizeof(float), circleData.positionsY.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // Previous position X
    glBindBuffer(GL_ARRAY_BUFFER, m_prevPositionXBuffer);
    glBufferData(GL_ARRAY_BUFFER, circleData.getCircleCount() * sizeof(float), circleData.previousPositionsX.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    // Previous position Y
    glBindBuffer(GL_ARRAY_BUFFER, m_prevPositionYBuffer);
    glBufferData(GL_ARRAY_BUFFER, circleData.getCircleCount() * sizeof(float), circleData.previousPositionsY.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    // Red component
    glBindBuffer(GL_ARRAY_BUFFER, m_redBuffer);
    glBufferData(GL_ARRAY_BUFFER, circleData.getCircleCount() * sizeof(float), circleData.r.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    // Green component
    glBindBuffer(GL_ARRAY_BUFFER, m_greenBuffer);
    glBufferData(GL_ARRAY_BUFFER, circleData.getCircleCount() * sizeof(float), circleData.g.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1);

    // Blue component
    glBindBuffer(GL_ARRAY_BUFFER, m_blueBuffer);
    glBufferData(GL_ARRAY_BUFFER, circleData.getCircleCount() * sizeof(float), circleData.b.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(7);
    glVertexAttribDivisor(7, 1);

    // Radius
    glBindBuffer(GL_ARRAY_BUFFER, m_radiusBuffer);
    glBufferData(GL_ARRAY_BUFFER, circleData.getCircleCount() * sizeof(float), circleData.radii.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(8);
    glVertexAttribDivisor(8, 1);

    // Outline width
    glBindBuffer(GL_ARRAY_BUFFER, m_outlineWidthBuffer);
    glBufferData(GL_ARRAY_BUFFER, circleData.getCircleCount() * sizeof(float), circleData.outlineWidths.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(9, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(9);
    glVertexAttribDivisor(9, 1);

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
    actualPhysicsFrequency = m_config.physicsFrequency;
    double fixedTimeStep = 1.0 / actualPhysicsFrequency;
    double accumulator = 0.0;

    int lastCircleCount = 0;

    int accumulatedCollisionChecks = 0;

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

        const float worldBoundX = scale * aspectRatio;
        const float worldBoundY = scale;

        m_engine.setWorldBounds(worldBoundX, worldBoundY);

        bool worldUpdated = false;

        // Fixed time step physics updates
        while (accumulator >= fixedTimeStep)
        {
            const double beforeStepTime = glfwGetTime();
            accumulatedCollisionChecks += m_engine.step(currentTime, fixedTimeStep);
            const float stepTime = glfwGetTime() - beforeStepTime;
            if (m_config.scalePhysics)
            {
                if (actualPhysicsFrequency > 10.0 && stepTime > fixedTimeStep)
                {
                    // Draw down physics resolution to keep fps up
                    actualPhysicsFrequency -= 1.0;
                    fixedTimeStep = 1.0 / actualPhysicsFrequency;
                }
                else if (actualPhysicsFrequency < m_config.physicsFrequency && stepTime < fixedTimeStep / 2.f)
                {
                    actualPhysicsFrequency += 1.0;
                    fixedTimeStep = 1.0 / actualPhysicsFrequency;
                }
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
            const int averageCollisionChecks = accumulatedCollisionChecks / stepCount;

            std::cout << std::endl;
            std::cout << "Circle count: " << m_engine.getCircleData().getCircleCount() << std::endl;
            std::cout << "Average FPS: " << std::fixed << std::setprecision(1) << fps << std::endl;
            std::cout << "Physics frequency: " << actualPhysicsFrequency << " Hz (" << fixedTimeStep * 1000.0 << " ms)" << std::endl;
            if (stepCount > 0)
            {
                std::cout << "Average step time: " << std::fixed << std::setprecision(2) << averageStepTime * 1000.0 << " ms" << std::endl;
                std::cout << "Average collision checks: " << averageCollisionChecks << std::endl;
            }
            // Uncomment to compare performance of features
            //std::cout << "Spatial partitioning is " << (m_engine.m_useSpatialPartitioning ? "ON" : "OFF") << std::endl;
            //m_engine.m_useSpatialPartitioning = !m_engine.m_useSpatialPartitioning;
            //std::cout << (m_engine.m_singleThreaded ? "Single" : "Multi") << "-threaded" << std::endl;
            //m_engine.m_singleThreaded = !m_engine.m_singleThreaded;

            frameCount = 0;
            accumulatedStepTime = 0.0;
            accumulatedCollisionChecks = 0;
            stepCount = 0;
            lastReportTime = currentTime;
        }

        // Clear the screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        const CircleData& circleData = m_engine.getCircleData();

        // Only update instance buffers when physics has been updated
        if (worldUpdated)
        {
            const int circleCount = circleData.getCircleCount();

            // Update each attribute buffer with new data
            glBindBuffer(GL_ARRAY_BUFFER, m_positionXBuffer);
            glBufferData(GL_ARRAY_BUFFER, circleCount * sizeof(float), circleData.positionsX.data(), GL_DYNAMIC_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER, m_positionYBuffer);
            glBufferData(GL_ARRAY_BUFFER, circleCount * sizeof(float), circleData.positionsY.data(), GL_DYNAMIC_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER, m_prevPositionXBuffer);
            glBufferData(GL_ARRAY_BUFFER, circleCount * sizeof(float), circleData.previousPositionsX.data(), GL_DYNAMIC_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER, m_prevPositionYBuffer);
            glBufferData(GL_ARRAY_BUFFER, circleCount * sizeof(float), circleData.previousPositionsY.data(), GL_DYNAMIC_DRAW);

            // These attributes never change, so only if circles were added
            if (circleCount != lastCircleCount)
            {
                glBindBuffer(GL_ARRAY_BUFFER, m_redBuffer);
                glBufferData(GL_ARRAY_BUFFER, circleCount * sizeof(float), circleData.r.data(), GL_DYNAMIC_DRAW);

                glBindBuffer(GL_ARRAY_BUFFER, m_greenBuffer);
                glBufferData(GL_ARRAY_BUFFER, circleCount * sizeof(float), circleData.g.data(), GL_DYNAMIC_DRAW);

                glBindBuffer(GL_ARRAY_BUFFER, m_blueBuffer);
                glBufferData(GL_ARRAY_BUFFER, circleCount * sizeof(float), circleData.b.data(), GL_DYNAMIC_DRAW);

                glBindBuffer(GL_ARRAY_BUFFER, m_radiusBuffer);
                glBufferData(GL_ARRAY_BUFFER, circleCount * sizeof(float), circleData.radii.data(), GL_DYNAMIC_DRAW);

                glBindBuffer(GL_ARRAY_BUFFER, m_outlineWidthBuffer);
                glBufferData(GL_ARRAY_BUFFER, circleCount * sizeof(float), circleData.outlineWidths.data(), GL_DYNAMIC_DRAW);

                lastCircleCount = circleCount;
            }

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
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, circleData.getCircleCount());
        glBindVertexArray(0);

        // Swap buffers and poll events
        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }

}

void Renderer::cleanUp()
{
    // Cleanup of all resources

    if (m_window) glfwDestroyWindow(m_window);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    if (m_vertexArray) glDeleteVertexArrays(1, &m_vertexArray);
    if (m_vertexBuffer) glDeleteBuffers(1, &m_vertexBuffer);
    if (m_indexBuffer) glDeleteBuffers(1, &m_indexBuffer);
    if (m_positionXBuffer) glDeleteBuffers(1, &m_positionXBuffer);
    if (m_positionYBuffer) glDeleteBuffers(1, &m_positionYBuffer);
    if (m_prevPositionXBuffer) glDeleteBuffers(1, &m_prevPositionXBuffer);
    if (m_prevPositionYBuffer) glDeleteBuffers(1, &m_prevPositionYBuffer);
    if (m_redBuffer) glDeleteBuffers(1, &m_redBuffer);
    if (m_greenBuffer) glDeleteBuffers(1, &m_greenBuffer);
    if (m_blueBuffer) glDeleteBuffers(1, &m_blueBuffer);
    if (m_radiusBuffer) glDeleteBuffers(1, &m_radiusBuffer);
    if (m_outlineWidthBuffer) glDeleteBuffers(1, &m_outlineWidthBuffer);
    if (m_circleShaderProgram) glDeleteProgram(m_circleShaderProgram);

    m_window = nullptr;
}

}